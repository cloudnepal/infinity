//
// Created by JinHai on 2022/9/26.
//

#pragma once

#include "planner/logical_node.h"

namespace infinity {

class LogicalProject: public LogicalNode {
public:
    LogicalProject(u64 node_id,
                   Vector<SharedPtr<BaseExpression>> expressions,
                   u64 projection_index)
        : LogicalNode(node_id, LogicalNodeType::kProjection),
        expressions_(std::move(expressions)),
        table_index_(projection_index)
        {}

    [[nodiscard]] Vector<ColumnBinding>
    GetColumnBindings() const final;

    String
    ToString(i64& space) final;

    inline String
    name() final {
        return "LogicalProject";
    }

    Vector<SharedPtr<BaseExpression>> expressions_{};

    u64 table_index_{};
};


}
