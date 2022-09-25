//
// Created by JinHai on 2022/9/25.
//

#include "logical_cross_product.h"
#include "common/value.h"

#include <sstream>

namespace infinity {

LogicalCrossProduct::LogicalCrossProduct(int64_t node_id,
                                         const std::shared_ptr<LogicalNode>& left,
                                         const std::shared_ptr<LogicalNode>& right)
                                         : LogicalNode(node_id, LogicalNodeType::kCrossProduct) {
    this->set_left_node(left);
    this->set_right_node(right);
}

std::string LogicalCrossProduct::ToString(uint64_t space) {
    std::stringstream ss;
    ss << std::string(space, ' ') << "CrossProduct: " << std::endl;
    ss << left_node_->ToString(space + TAB);
    ss << right_node_->ToString(space + TAB);
    return ss.str();
}

}