//
// Created by JinHai on 2022/7/28.
//

#include "physical_cross_product.h"

namespace infinity {

void
PhysicalCrossProduct::Init() {
    ExecutorAssert(left()->outputs().size() == 1, "Left Input table count isn't matched.");
    ExecutorAssert(right()->outputs().size() == 1, "Right Input table count isn't matched.");

    for(const auto& input_table: left()->outputs()) {
        left_table_ = input_table.second;
        left_table_index_ = input_table.first;
    }

    for(const auto& input_table: left()->outputs()) {
        right_table_ = input_table.second;
        right_table_index_ = input_table.first;
    }
}

void
PhysicalCrossProduct::Execute(std::shared_ptr<QueryContext>& query_context) {
    Vector<SharedPtr<ColumnDef>> columns_def;
    SizeT left_column_count = left_table_->ColumnCount();
    SizeT right_column_count = right_table_->ColumnCount();
    columns_def.reserve(left_column_count + right_column_count);

    {
        i64 column_idx{0};
        const Vector<SharedPtr<ColumnDef>> &left_column_defs = left_table_->definition_ptr_->columns();
        for (const SharedPtr<ColumnDef> &input_col_def: left_column_defs) {
            SharedPtr<ColumnDef> output_col_def = ColumnDef::Make(input_col_def->name(),
                                                                  column_idx,
                                                                  input_col_def->type(),
                                                                  input_col_def->constrains());
            columns_def.emplace_back(output_col_def);
            ++column_idx;
        }

        const Vector<SharedPtr<ColumnDef>> &right_column_defs = right_table_->definition_ptr_->columns();
        for (const SharedPtr<ColumnDef> &input_col_def: right_column_defs) {
            SharedPtr<ColumnDef> output_col_def = ColumnDef::Make(input_col_def->name(),
                                                                  column_idx,
                                                                  input_col_def->type(),
                                                                  input_col_def->constrains());
            columns_def.emplace_back(output_col_def);
            ++column_idx;
        }
    }

    SharedPtr<TableDef> cross_product_table_def = TableDef::Make("cross_product", columns_def, false);
    SharedPtr<Table> cross_product_table = Table::Make(cross_product_table_def, TableType::kCrossProduct);

    // Loop left table and scan right table
    SizeT left_block_count = left_table_->DataBlockCount();
    SizeT right_block_count = right_table_->DataBlockCount();
    for(const SharedPtr<DataBlock>& left_block: left_table_->data_blocks_) {
        for(const SharedPtr<DataBlock>& right_block: right_table_->data_blocks_) {
            // each row of left block will generate the constant column vectors and corresponding right column vectors
            SizeT output_row_count = right_block->row_count();

            SizeT row_count = left_block->row_count();
            for(SizeT row_idx = 0; row_idx < row_count; ++ row_idx) {
                // left block column vectors
                Vector<SharedPtr<ColumnVector>> output_columns;
                output_columns.reserve(left_column_count + right_column_count);
                for(SizeT column_idx = 0; column_idx < left_column_count; ++ column_idx) {
                    const SharedPtr<ColumnVector>& left_column_vector = left_block->column_vectors[column_idx];
                    SharedPtr<ColumnVector> column_vector = ColumnVector::Make(left_column_vector->data_type());
                    column_vector->Initialize(ColumnVectorType::kConstant, *left_column_vector, row_idx, row_idx + 1);
                    output_columns.emplace_back(column_vector);
                }

                for(SizeT column_idx = 0; column_idx < right_column_count; ++ column_idx) {
                    const SharedPtr<ColumnVector>& right_column_vector = right_block->column_vectors[column_idx];
                }
            }



            for(const auto& input_column_vector: left_block->column_vectors) {
                SharedPtr<ColumnVector> column_vector = ColumnVector::Make(input_column_vector->data_type());
            }

        }
    }
}

}


