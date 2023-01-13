//
// Created by jinhai on 23-1-7.
//

#include <gtest/gtest.h>
#include "base_test.h"
#include "common/column_vector/column_vector.h"
#include "common/types/value.h"
#include "main/logger.h"
#include "main/stats/global_resource_usage.h"
#include "common/types/info/varchar_info.h"
#include "storage/catalog.h"
#include "function/scalar/greater.h"
#include "function/scalar_function_set.h"
#include "expression/column_expression.h"

class GreaterFunctionsTest : public BaseTest {
    void
    SetUp() override {
        infinity::Logger::Initialize();
        infinity::GlobalResourceUsage::Init();
    }

    void
    TearDown() override {
        infinity::Logger::Shutdown();
        EXPECT_EQ(infinity::GlobalResourceUsage::GetObjectCount(), 0);
        EXPECT_EQ(infinity::GlobalResourceUsage::GetRawMemoryCount(), 0);
        infinity::GlobalResourceUsage::UnInit();
    }
};

TEST_F(GreaterFunctionsTest, greater_func) {
    using namespace infinity;

    UniquePtr<Catalog> catalog_ptr = MakeUnique<Catalog>();

    RegisterGreaterFunction(catalog_ptr);

    SharedPtr<FunctionSet> function_set = catalog_ptr->GetFunctionSetByName(">");
    EXPECT_EQ(function_set->type_, FunctionType::kScalar);
    SharedPtr<ScalarFunctionSet> scalar_function_set = std::static_pointer_cast<ScalarFunctionSet>(function_set);

    {
        Vector<SharedPtr<BaseExpression>> inputs;

        DataType data_type(LogicalType::kTinyInt);
        DataType result_type(LogicalType::kBoolean);
        SharedPtr<ColumnExpression> col1_expr_ptr = MakeShared<ColumnExpression>(data_type,
                                                                                 "t1",
                                                                                 "c1",
                                                                                 0,
                                                                                 0);
        SharedPtr<ColumnExpression> col2_expr_ptr = MakeShared<ColumnExpression>(data_type,
                                                                                 "t1",
                                                                                 "c2",
                                                                                 1,
                                                                                 0);

        inputs.emplace_back(col1_expr_ptr);
        inputs.emplace_back(col2_expr_ptr);

        ScalarFunction func = scalar_function_set->GetMostMatchFunction(inputs);
        EXPECT_STREQ(">(TinyInt, TinyInt)->Boolean", func.ToString().c_str());

        std::vector<DataType> column_types;
        column_types.emplace_back(data_type);
        column_types.emplace_back(data_type);

        size_t row_count = DEFAULT_VECTOR_SIZE;

        DataBlock data_block;
        data_block.Init(column_types);

        for (size_t i = 0; i < row_count; ++i) {
            data_block.AppendValue(0, Value::MakeTinyInt(static_cast<i8>(i)));
            data_block.AppendValue(1, Value::MakeTinyInt(static_cast<i8>(i + i)));
        }
        data_block.Finalize();

        for (size_t i = 0; i < row_count; ++i) {
            Value v1 = data_block.GetValue(0, i);
            Value v2 = data_block.GetValue(1, i);
            EXPECT_EQ(v1.type_.type(), LogicalType::kTinyInt);
            EXPECT_EQ(v2.type_.type(), LogicalType::kTinyInt);
            EXPECT_EQ(v1.value_.tiny_int, static_cast<i8>(i));
            EXPECT_EQ(v2.value_.tiny_int, static_cast<i8>(i + i));
        }

        ColumnVector result(result_type);
        result.Initialize();
        func.function_(data_block, result);

        for (size_t i = 0; i < row_count; ++i) {
            Value v = result.GetValue(i);
            EXPECT_EQ(v.type_.type(), LogicalType::kBoolean);
            if(static_cast<i8>(i) > static_cast<i8>(i + i)) {
                EXPECT_EQ(v.value_.boolean, true);
            } else {
                EXPECT_EQ(v.value_.boolean, false);
            }
        }
    }

    {
        Vector<SharedPtr<BaseExpression>> inputs;

        DataType data_type(LogicalType::kSmallInt);
        DataType result_type(LogicalType::kBoolean);
        SharedPtr<ColumnExpression> col1_expr_ptr = MakeShared<ColumnExpression>(data_type,
                                                                                 "t1",
                                                                                 "c1",
                                                                                 0,
                                                                                 0);
        SharedPtr<ColumnExpression> col2_expr_ptr = MakeShared<ColumnExpression>(data_type,
                                                                                 "t1",
                                                                                 "c2",
                                                                                 1,
                                                                                 0);

        inputs.emplace_back(col1_expr_ptr);
        inputs.emplace_back(col2_expr_ptr);

        ScalarFunction func = scalar_function_set->GetMostMatchFunction(inputs);
        EXPECT_STREQ(">(SmallInt, SmallInt)->Boolean", func.ToString().c_str());

        std::vector<DataType> column_types;
        column_types.emplace_back(data_type);
        column_types.emplace_back(data_type);

        size_t row_count = DEFAULT_VECTOR_SIZE;

        DataBlock data_block;
        data_block.Init(column_types);

        for (size_t i = 0; i < row_count; ++i) {
            data_block.AppendValue(0, Value::MakeSmallInt(static_cast<i16>(i)));
            data_block.AppendValue(1, Value::MakeSmallInt(static_cast<i16>(i / 2)));
        }
        data_block.Finalize();

        for (size_t i = 0; i < row_count; ++i) {
            Value v1 = data_block.GetValue(0, i);
            Value v2 = data_block.GetValue(1, i);
            EXPECT_EQ(v1.type_.type(), LogicalType::kSmallInt);
            EXPECT_EQ(v2.type_.type(), LogicalType::kSmallInt);
            EXPECT_EQ(v1.value_.small_int, static_cast<i16>(i));
            EXPECT_EQ(v2.value_.small_int, static_cast<i16>(i / 2));
        }

        ColumnVector result(result_type);
        result.Initialize();
        func.function_(data_block, result);

        for (size_t i = 0; i < row_count; ++i) {
            Value v = result.GetValue(i);
            EXPECT_EQ(v.type_.type(), LogicalType::kBoolean);
            if(static_cast<i16>(i) > static_cast<i16>(i / 2)) {
                EXPECT_EQ(v.value_.boolean, true);
            } else {
                EXPECT_EQ(v.value_.boolean, false);
            }
        }
    }

    {
        Vector<SharedPtr<BaseExpression>> inputs;

        DataType data_type(LogicalType::kInteger);
        DataType result_type(LogicalType::kBoolean);
        SharedPtr<ColumnExpression> col1_expr_ptr = MakeShared<ColumnExpression>(data_type,
                                                                                 "t1",
                                                                                 "c1",
                                                                                 0,
                                                                                 0);
        SharedPtr<ColumnExpression> col2_expr_ptr = MakeShared<ColumnExpression>(data_type,
                                                                                 "t1",
                                                                                 "c2",
                                                                                 1,
                                                                                 0);

        inputs.emplace_back(col1_expr_ptr);
        inputs.emplace_back(col2_expr_ptr);

        ScalarFunction func = scalar_function_set->GetMostMatchFunction(inputs);
        EXPECT_STREQ(">(Integer, Integer)->Boolean", func.ToString().c_str());

        std::vector<DataType> column_types;
        column_types.emplace_back(data_type);
        column_types.emplace_back(data_type);

        size_t row_count = DEFAULT_VECTOR_SIZE;

        DataBlock data_block;
        data_block.Init(column_types);

        for (size_t i = 0; i < row_count; ++i) {
            data_block.AppendValue(0, Value::MakeInt(static_cast<i32>(i)));
            data_block.AppendValue(1, Value::MakeInt(static_cast<i32>(i / 2)));
        }
        data_block.Finalize();

        for (size_t i = 0; i < row_count; ++i) {
            Value v1 = data_block.GetValue(0, i);
            Value v2 = data_block.GetValue(1, i);
            EXPECT_EQ(v1.type_.type(), LogicalType::kInteger);
            EXPECT_EQ(v2.type_.type(), LogicalType::kInteger);
            EXPECT_EQ(v1.value_.integer, static_cast<i32>(i));
            EXPECT_EQ(v2.value_.integer, static_cast<i32>(i / 2));
        }

        ColumnVector result(result_type);
        result.Initialize();
        func.function_(data_block, result);

        for (size_t i = 0; i < row_count; ++i) {
            Value v = result.GetValue(i);
            EXPECT_EQ(v.type_.type(), LogicalType::kBoolean);
            if(static_cast<i32>(i) > static_cast<i32>(i / 2)) {
                EXPECT_EQ(v.value_.boolean, true);
            } else {
                EXPECT_EQ(v.value_.boolean, false);
            }
        }
    }

    {
        Vector<SharedPtr<BaseExpression>> inputs;

        DataType data_type(LogicalType::kBigInt);
        DataType result_type(LogicalType::kBoolean);
        SharedPtr<ColumnExpression> col1_expr_ptr = MakeShared<ColumnExpression>(data_type,
                                                                                 "t1",
                                                                                 "c1",
                                                                                 0,
                                                                                 0);
        SharedPtr<ColumnExpression> col2_expr_ptr = MakeShared<ColumnExpression>(data_type,
                                                                                 "t1",
                                                                                 "c2",
                                                                                 1,
                                                                                 0);

        inputs.emplace_back(col1_expr_ptr);
        inputs.emplace_back(col2_expr_ptr);

        ScalarFunction func = scalar_function_set->GetMostMatchFunction(inputs);
        EXPECT_STREQ(">(BigInt, BigInt)->Boolean", func.ToString().c_str());

        std::vector<DataType> column_types;
        column_types.emplace_back(data_type);
        column_types.emplace_back(data_type);

        size_t row_count = DEFAULT_VECTOR_SIZE;

        DataBlock data_block;
        data_block.Init(column_types);

        for (size_t i = 0; i < row_count; ++i) {
            data_block.AppendValue(0, Value::MakeBigInt(static_cast<i64>(i)));
            data_block.AppendValue(1, Value::MakeBigInt(static_cast<i64>(i / 2)));
        }
        data_block.Finalize();

        for (size_t i = 0; i < row_count; ++i) {
            Value v1 = data_block.GetValue(0, i);
            Value v2 = data_block.GetValue(1, i);
            EXPECT_EQ(v1.type_.type(), LogicalType::kBigInt);
            EXPECT_EQ(v2.type_.type(), LogicalType::kBigInt);
            EXPECT_EQ(v1.value_.big_int, static_cast<i64>(i));
            EXPECT_EQ(v2.value_.big_int, static_cast<i64>(i / 2));
        }

        ColumnVector result(result_type);
        result.Initialize();
        func.function_(data_block, result);

        for (size_t i = 0; i < row_count; ++i) {
            Value v = result.GetValue(i);
            EXPECT_EQ(v.type_.type(), LogicalType::kBoolean);
            if(static_cast<i64>(i) > static_cast<i64>(i / 2)) {
                EXPECT_EQ(v.value_.boolean, true);
            } else {
                EXPECT_EQ(v.value_.boolean, false);
            }
        }
    }

    {
        Vector<SharedPtr<BaseExpression>> inputs;

        DataType data_type(LogicalType::kHugeInt);
        DataType result_type(LogicalType::kBoolean);
        SharedPtr<ColumnExpression> col1_expr_ptr = MakeShared<ColumnExpression>(data_type,
                                                                                 "t1",
                                                                                 "c1",
                                                                                 0,
                                                                                 0);
        SharedPtr<ColumnExpression> col2_expr_ptr = MakeShared<ColumnExpression>(data_type,
                                                                                 "t1",
                                                                                 "c2",
                                                                                 1,
                                                                                 0);

        inputs.emplace_back(col1_expr_ptr);
        inputs.emplace_back(col2_expr_ptr);

        ScalarFunction func = scalar_function_set->GetMostMatchFunction(inputs);
        EXPECT_STREQ(">(HugeInt, HugeInt)->Boolean", func.ToString().c_str());

        std::vector<DataType> column_types;
        column_types.emplace_back(data_type);
        column_types.emplace_back(data_type);

        size_t row_count = DEFAULT_VECTOR_SIZE;

        DataBlock data_block;
        data_block.Init(column_types);

        for (size_t i = 0; i < row_count; ++i) {
            data_block.AppendValue(0, Value::MakeHugeInt(HugeIntT(static_cast<i64>(i), static_cast<i64>(i))));
            data_block.AppendValue(1, Value::MakeHugeInt(HugeIntT(static_cast<i64>(i / 2), static_cast<i64>(i / 2))));
        }
        data_block.Finalize();

        for (size_t i = 0; i < row_count; ++i) {
            Value v1 = data_block.GetValue(0, i);
            Value v2 = data_block.GetValue(1, i);
            EXPECT_EQ(v1.type_.type(), LogicalType::kHugeInt);
            EXPECT_EQ(v2.type_.type(), LogicalType::kHugeInt);
            EXPECT_EQ(v1.value_.huge_int, HugeIntT(static_cast<i64>(i), static_cast<i64>(i)));
            EXPECT_EQ(v2.value_.huge_int, HugeIntT(static_cast<i64>(i / 2), static_cast<i64>(i / 2)));
        }

        ColumnVector result(result_type);
        result.Initialize();
        func.function_(data_block, result);

        for (size_t i = 0; i < row_count; ++i) {
            Value v = result.GetValue(i);
            EXPECT_EQ(v.type_.type(), LogicalType::kBoolean);
            if(HugeIntT(static_cast<i64>(i), static_cast<i64>(i)) > HugeIntT(static_cast<i64>(i / 2), static_cast<i64>(i / 2))) {
                EXPECT_EQ(v.value_.boolean, true);
            } else {
                EXPECT_EQ(v.value_.boolean, false);
            }
        }
    }

    {
        Vector<SharedPtr<BaseExpression>> inputs;

        DataType data_type(LogicalType::kFloat);
        DataType result_type(LogicalType::kBoolean);
        SharedPtr<ColumnExpression> col1_expr_ptr = MakeShared<ColumnExpression>(data_type,
                                                                                 "t1",
                                                                                 "c1",
                                                                                 0,
                                                                                 0);
        SharedPtr<ColumnExpression> col2_expr_ptr = MakeShared<ColumnExpression>(data_type,
                                                                                 "t1",
                                                                                 "c2",
                                                                                 1,
                                                                                 0);

        inputs.emplace_back(col1_expr_ptr);
        inputs.emplace_back(col2_expr_ptr);

        ScalarFunction func = scalar_function_set->GetMostMatchFunction(inputs);
        EXPECT_STREQ(">(Float, Float)->Boolean", func.ToString().c_str());

        std::vector<DataType> column_types;
        column_types.emplace_back(data_type);
        column_types.emplace_back(data_type);

        size_t row_count = DEFAULT_VECTOR_SIZE;

        DataBlock data_block;
        data_block.Init(column_types);

        for (size_t i = 0; i < row_count; ++i) {
            data_block.AppendValue(0, Value::MakeFloat(static_cast<f32>(i)));
            data_block.AppendValue(1, Value::MakeFloat(static_cast<f32>(i) / 2));
        }
        data_block.Finalize();

        for (size_t i = 0; i < row_count; ++i) {
            Value v1 = data_block.GetValue(0, i);
            Value v2 = data_block.GetValue(1, i);
            EXPECT_EQ(v1.type_.type(), LogicalType::kFloat);
            EXPECT_EQ(v2.type_.type(), LogicalType::kFloat);
            EXPECT_FLOAT_EQ(v1.value_.float32, static_cast<f32>(i));
            EXPECT_FLOAT_EQ(v2.value_.float32, static_cast<f32>(i) / 2);
        }

        ColumnVector result(result_type);
        result.Initialize();
        func.function_(data_block, result);

        for (size_t i = 0; i < row_count; ++i) {
            Value v = result.GetValue(i);
            EXPECT_EQ(v.type_.type(), LogicalType::kBoolean);
            if(static_cast<f32>(i) > (static_cast<f32>(i) / 2)) {
                EXPECT_EQ(v.value_.boolean, true);
            } else {
                EXPECT_EQ(v.value_.boolean, false);
            }
        }
    }

    {
        Vector<SharedPtr<BaseExpression>> inputs;

        DataType data_type(LogicalType::kDouble);
        DataType result_type(LogicalType::kBoolean);
        SharedPtr<ColumnExpression> col1_expr_ptr = MakeShared<ColumnExpression>(data_type,
                                                                                 "t1",
                                                                                 "c1",
                                                                                 0,
                                                                                 0);
        SharedPtr<ColumnExpression> col2_expr_ptr = MakeShared<ColumnExpression>(data_type,
                                                                                 "t1",
                                                                                 "c2",
                                                                                 1,
                                                                                 0);

        inputs.emplace_back(col1_expr_ptr);
        inputs.emplace_back(col2_expr_ptr);

        ScalarFunction func = scalar_function_set->GetMostMatchFunction(inputs);
        EXPECT_STREQ(">(Double, Double)->Boolean", func.ToString().c_str());

        std::vector<DataType> column_types;
        column_types.emplace_back(data_type);
        column_types.emplace_back(data_type);

        size_t row_count = DEFAULT_VECTOR_SIZE;

        DataBlock data_block;
        data_block.Init(column_types);

        for (size_t i = 0; i < row_count; ++i) {
            data_block.AppendValue(0, Value::MakeDouble(static_cast<f64>(i)));
            data_block.AppendValue(1, Value::MakeDouble(static_cast<f64>(i) / 2));
        }
        data_block.Finalize();

        for (size_t i = 0; i < row_count; ++i) {
            Value v1 = data_block.GetValue(0, i);
            Value v2 = data_block.GetValue(1, i);
            EXPECT_EQ(v1.type_.type(), LogicalType::kDouble);
            EXPECT_EQ(v2.type_.type(), LogicalType::kDouble);
            EXPECT_FLOAT_EQ(v1.value_.float64, static_cast<f64>(i));
            EXPECT_FLOAT_EQ(v2.value_.float64, static_cast<f64>(i) / 2);
        }

        ColumnVector result(result_type);
        result.Initialize();
        func.function_(data_block, result);

        for (size_t i = 0; i < row_count; ++i) {
            Value v = result.GetValue(i);
            EXPECT_EQ(v.type_.type(), LogicalType::kBoolean);
            if(static_cast<f64>(i) > (static_cast<f64>(i) / 2)) {
                EXPECT_EQ(v.value_.boolean, true);
            } else {
                EXPECT_EQ(v.value_.boolean, false);
            }
        }
    }

    {
        Vector<SharedPtr<BaseExpression>> inputs;

        SharedPtr<TypeInfo> type_info_ptr1 = VarcharInfo::Make(64);
        SharedPtr<TypeInfo> type_info_ptr2 = VarcharInfo::Make(128);

        DataType data_type1(LogicalType::kVarchar, type_info_ptr1);
        DataType data_type2(LogicalType::kVarchar, type_info_ptr2);
        DataType result_type(LogicalType::kBoolean);
        SharedPtr<ColumnExpression> col1_expr_ptr = MakeShared<ColumnExpression>(data_type1,
                                                                                 "t1",
                                                                                 "c1",
                                                                                 0,
                                                                                 0);
        SharedPtr<ColumnExpression> col2_expr_ptr = MakeShared<ColumnExpression>(data_type2,
                                                                                 "t1",
                                                                                 "c2",
                                                                                 1,
                                                                                 0);

        inputs.emplace_back(col1_expr_ptr);
        inputs.emplace_back(col2_expr_ptr);

        ScalarFunction func = scalar_function_set->GetMostMatchFunction(inputs);
        EXPECT_STREQ(">(Varchar, Varchar)->Boolean", func.ToString().c_str());

        std::vector<DataType> column_types;
        column_types.emplace_back(data_type1);
        column_types.emplace_back(data_type2);

        size_t row_count = DEFAULT_VECTOR_SIZE;

        DataBlock data_block;
        data_block.Init(column_types);

        for (size_t i = 0; i < row_count; ++i) {
            if(i % 2 == 0) {
                data_block.AppendValue(0, Value::MakeVarchar("Helloworld" + std::to_string(i), type_info_ptr1));
                data_block.AppendValue(1, Value::MakeVarchar("Helloworld" + std::to_string(i), type_info_ptr2));
            } else {
                data_block.AppendValue(0, Value::MakeVarchar("Helloworld" + std::to_string(i), type_info_ptr1));
                data_block.AppendValue(1, Value::MakeVarchar("helloworld" + std::to_string(i), type_info_ptr2));
            }
        }
        data_block.Finalize();

        for (size_t i = 0; i < row_count; ++i) {
            Value v1 = data_block.GetValue(0, i);
            Value v2 = data_block.GetValue(1, i);
            EXPECT_EQ(v1.type_.type(), LogicalType::kVarchar);
            EXPECT_EQ(v2.type_.type(), LogicalType::kVarchar);
            if(i % 2 == 0) {
                EXPECT_EQ(v1.value_.varchar.ToString(), "Helloworld" + std::to_string(i));
                EXPECT_EQ(v2.value_.varchar.ToString(), "Helloworld" + std::to_string(i));
            } else {
                EXPECT_EQ(v1.value_.varchar.ToString(), "Helloworld" + std::to_string(i));
                EXPECT_EQ(v2.value_.varchar.ToString(), "helloworld" + std::to_string(i));
            }
        }

        ColumnVector result(result_type);
        result.Initialize();
        func.function_(data_block, result);

        for (size_t i = 0; i < row_count; ++i) {
            Value v = result.GetValue(i);
            EXPECT_EQ(v.type_.type(), LogicalType::kBoolean);
            if(i % 2 == 0) {
                String s1 = "Helloworld" + std::to_string(i);
                String s2 = "Helloworld" + std::to_string(i);
                if(s1 > s2) {
                    EXPECT_EQ(v.value_.boolean, true);
                } else {
                    EXPECT_EQ(v.value_.boolean, false);
                }
            } else {
                String s1 = "Helloworld" + std::to_string(i);
                String s2 = "helloworld" + std::to_string(i);
                if(s1 > s2) {
                    EXPECT_EQ(v.value_.boolean, true);
                } else {
                    EXPECT_EQ(v.value_.boolean, false);
                }
            }
        }
    }
}