// Copyright(C) 2023 InfiniFlow, Inc. All rights reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     https://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

module;

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-variable"
#pragma clang diagnostic ignored "-Wunused-but-set-variable"
#pragma clang diagnostic ignored "-Wmissing-field-initializers"
#pragma clang diagnostic ignored "-W#pragma-messages"

#include <ctpl_stl.h>

#pragma clang diagnostic pop

#include <filesystem>
#include <string.h>

module memory_indexer;

import stl;
import memory_pool;
import index_defines;
import index_config;
import posting_writer;
import column_vector;
import analyzer;
import analyzer_pool;
import term;
import column_inverter;
import invert_task;
import third_party;
import ring;
import external_sort_merger;
import local_file_system;
import file_writer;
import term_meta;
import fst;

namespace infinity {
constexpr int MAX_TUPLE_LENGTH = 1024; // we assume that analyzed term, together with docid/offset info, will never exceed such length

bool MemoryIndexer::KeyComp::operator()(const String &lhs, const String &rhs) const {
    int ret = strcmp(lhs.c_str(), rhs.c_str());
    return ret < 0;
}

MemoryIndexer::MemoryIndexer(u64 column_id,
                             const InvertedIndexConfig &index_config,
                             SharedPtr<MemoryPool> byte_slice_pool,
                             SharedPtr<RecyclePool> buffer_pool,
                             ThreadPool &thread_pool)
    : column_id_(column_id), index_config_(index_config), byte_slice_pool_(byte_slice_pool), buffer_pool_(buffer_pool), thread_pool_(thread_pool),
      ring_inverted_(10UL), ring_sorted_(10UL) {
    posting_store_ = MakeUnique<PostingTable>(KeyComp(), byte_slice_pool_.get());
    SetAnalyzer();

    // TODO need get base directory from constructor
    String index_dir = index_config_.GetIndexName();
    Path path = Path(index_dir) / std::to_string(column_id);
    spill_full_path_ = path.string() + ".merge";
}

MemoryIndexer::~MemoryIndexer() { Reset(); }

void MemoryIndexer::SetIndexMode(IndexMode index_mode) { index_mode_ = index_mode; }

void MemoryIndexer::SetAnalyzer() {
    String analyzer = index_config_.GetAnalyzer(column_id_);
    analyzer_ = AnalyzerPool::instance().Get(analyzer);
    jieba_specialize_ = analyzer.compare("chinese") == 0 ? true : false;
}

void MemoryIndexer::Insert(const ColumnVector &column_vector, u32 row_offset, u32 row_count, RowID row_id_begin) {
    {
        std::unique_lock<std::mutex> lock(mutex_);
        inflight_tasks_++;
    }
    u64 seq_inserted = seq_inserted_++;
    auto task = MakeShared<BatchInvertTask>(seq_inserted, column_vector, row_offset, row_count, row_id_begin);
    auto func = [this, &task](int id) {
        auto inverter = MakeShared<ColumnInverter>(*this);
        inverter->InvertColumn(task->column_vector_, task->row_offset_, task->row_count_, task->row_id_begin_);
        this->ring_inverted_.Put(task->task_seq_, inverter);
    };
    thread_pool_.push(func);
}

void MemoryIndexer::Commit() {
    thread_pool_.push([this](int id) {
        Vector<SharedPtr<ColumnInverter>> inverters;
        u64 seq_commit = this->ring_inverted_.GetBatch(inverters);
        SizeT num = inverters.size();
        for (SizeT i = 1; i < num; i++) {
            inverters[0]->Merge(*inverters[i]);
        }
        inverters[0]->Sort();
        this->ring_sorted_.Put(seq_commit, inverters[0]);
        this->ring_sorted_.Iterate([](SharedPtr<ColumnInverter> &inverter) { inverter->GeneratePosting(); });
        {
            std::unique_lock<std::mutex> lock(mutex_);
            inflight_tasks_ -= num;
            if (inflight_tasks_ == 0) {
                cv_.notify_all();
            }
        }
    });
}

MemoryIndexer::PostingPtr MemoryIndexer::GetOrAddPosting(const TermKey &term) {
    MemoryIndexer::PostingTable::Iterator iter = posting_store_->Find(term);
    if (iter != posting_store_->End())
        return iter.Value();
    else {
        MemoryIndexer::PostingPtr posting =
            MakeShared<PostingWriter>(byte_slice_pool_.get(), buffer_pool_.get(), index_config_.GetPostingFormatOption());
        posting_store_->Insert(term, posting);
        return posting;
    }
}

void MemoryIndexer::Reset() {
    if (posting_store_.get()) {
        for (auto it = posting_store_->Begin(); it != posting_store_->End(); ++it) {
            // delete it.getData();
        }
        posting_store_->Clear();
    }
    thread_pool_.stop(true);
    cv_.notify_all();
}

void MemoryIndexer::OfflineDump() {
    // Steps of offline dump:
    // 1. External sort merge
    // 2. Generate posting
    // 3. Dump disk segment data
    FinalSpillFile();
    SortMerger<TermTuple, u16> *merger = new SortMerger<TermTuple, u16>(spill_full_path_.c_str(), num_runs_, 100000000, 2);
    merger->Run();
    delete merger;
    FILE *f = fopen(spill_full_path_.c_str(), "r");
    u64 count;
    fread((char *)&count, sizeof(u64), 1, f);

    String index_prefix; /// TODO, to be integrated
    LocalFileSystem fs;
    String posting_file = index_prefix + POSTING_SUFFIX;
    SharedPtr<FileWriter> posting_file_writer = MakeShared<FileWriter>(fs, posting_file, 128000);
    String dict_file = index_prefix + DICT_SUFFIX;
    SharedPtr<FileWriter> dict_file_writer = MakeShared<FileWriter>(fs, dict_file, 128000);
    TermMetaDumper term_meta_dumpler(index_config_.GetPostingFormatOption());

    String fst_file = index_prefix + DICT_SUFFIX + ".fst";
    std::ofstream ofs(fst_file.c_str(), std::ios::binary | std::ios::trunc);
    OstreamWriter wtr(ofs);
    FstBuilder builder(wtr);

    std::string_view last_term;
    u32 last_term_pos = 0;
    u32 last_doc_id = INVALID_DOCID;
    u16 record_length;
    Deque<String> term_buffer;
    char buf[MAX_TUPLE_LENGTH];
    UniquePtr<PostingWriter> posting;
    SizeT term_meta_offset = 0;
    for (u64 i = 0; i < count; ++i) {
        fread(&record_length, sizeof(u16), 1, f);
        fread(buf, record_length, 1, f);
        TermTuple tuple(buf, record_length);
        if (tuple.term_ != last_term) {
            term_buffer.push_back(String(tuple.term_));
            std::string_view view(term_buffer.back());
            last_term.swap(view);
            if (posting.get()) {
                TermMeta term_meta(posting->GetDF(), posting->GetTotalTF());
                posting->Dump(posting_file_writer, term_meta);
                term_meta_dumpler.Dump(dict_file_writer, term_meta);
                builder.Insert((u8 *)last_term.data(), last_term.length(), term_meta_offset);
                term_meta_offset = dict_file_writer->TotalWrittenBytes();
            }
            posting = MakeUnique<PostingWriter>(byte_slice_pool_.get(), buffer_pool_.get(), index_config_.GetPostingFormatOption());
        }
        if (last_doc_id != tuple.doc_id_) {
            last_doc_id = tuple.doc_id_;
            posting->EndDocument(last_doc_id, 0);
        }
        if (tuple.term_pos_ != last_term_pos) {
            last_term_pos = tuple.term_pos_;
            posting->AddPosition(last_term_pos);
        }
    }
    if (posting->GetDF() > 0) {
        TermMeta term_meta(posting->GetDF(), posting->GetTotalTF());
        posting->Dump(posting_file_writer, term_meta);
        term_meta_dumpler.Dump(dict_file_writer, term_meta);
        builder.Insert((u8 *)last_term.data(), last_term.length(), term_meta_offset);
    }
    dict_file_writer->Sync();
    builder.Finish();
    fs.AppendFile(dict_file, fst_file);
    fs.DeleteFile(fst_file);

    num_runs_ = 0;
    std::filesystem::remove(spill_full_path_);
}

void MemoryIndexer::FinalSpillFile() {
    fseek(spill_file_handle_, 0, SEEK_SET);
    fwrite(&tuple_count_, sizeof(u64), 1, spill_file_handle_);
    fclose(spill_file_handle_);
    tuple_count_ = 0;
    spill_file_handle_ = nullptr;
}

void MemoryIndexer::PrepareSpillFile() {
    spill_file_handle_ = fopen(spill_full_path_.c_str(), "w");
    fwrite(&tuple_count_, sizeof(u64), 1, spill_file_handle_);
}

} // namespace infinity