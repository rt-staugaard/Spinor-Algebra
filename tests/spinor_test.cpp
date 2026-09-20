#include <gtest/gtest.h>
#include "../lib/spinor_tree_arithmetic.hpp"

TEST(MakeSumTest, IdenticalSum){
    Expression A = Expression('a', 3, 1);

    Expression Sum_2A = A + A;

    ASSERT_NE(Sum_2A.node_ptr, nullptr);
    auto node = Sum_2A.node_ptr;
    
    EXPECT_EQ(node->optype, OpType::Single);
    EXPECT_DOUBLE_EQ(node->scalar, 2.0);
    EXPECT_EQ(node->payload, A.node_ptr->payload);
    EXPECT_EQ(node->children.size(), 0);
}

TEST(MakeSumTest, Cancellation){
    Expression A = Expression('a', 3, 1);

    Expression Sum_0 = A - A;

    ASSERT_NE(Sum_0.node_ptr, nullptr);
    auto node = Sum_0.node_ptr;
    
    EXPECT_EQ(node->optype, OpType::Scalar);
    EXPECT_DOUBLE_EQ(node->scalar, 0.0);
    EXPECT_TRUE(std::holds_alternative<std::monostate>(node->payload));
    EXPECT_EQ(node->children.size(), 0);
}

TEST(MakeSumTest, BasicSum){
    Expression A = Expression('a', 3, 1);
    Expression B = Expression('a', 4, 2);

    Expression Sum_AB = A + B;

    ASSERT_NE(Sum_AB.node_ptr, nullptr);
    auto node = Sum_AB.node_ptr;
    
    EXPECT_EQ(node->optype, OpType::Sum);
    EXPECT_DOUBLE_EQ(node->scalar, 1.0);
    EXPECT_TRUE(std::holds_alternative<std::monostate>(node->payload));
    EXPECT_EQ(node->children.size(), 2);
    EXPECT_EQ(*node->children[0], *A.node_ptr);
    EXPECT_EQ(*node->children[1], *B.node_ptr);
}

TEST(MakeSumTest, MultipleSums){
    Expression A = Expression('a', 3, 1);
    Expression C = Expression('s', 2, 3);

    Expression Sum_ACA = A + C + A;

    ASSERT_NE(Sum_ACA.node_ptr, nullptr);
    auto node = Sum_ACA.node_ptr;
    
    EXPECT_EQ(node->optype, OpType::Sum);
    EXPECT_DOUBLE_EQ(node->scalar, 1.0);
    EXPECT_TRUE(std::holds_alternative<std::monostate>(node->payload));
    EXPECT_EQ(node->children.size(), 2);
    EXPECT_EQ(node->children[0]->payload, A.node_ptr->payload);
    EXPECT_EQ(node->children[0]->scalar, 2.0);
    EXPECT_EQ(node->children[1]->payload, C.node_ptr->payload);
    EXPECT_EQ(node->children[1]->scalar, -1.0);
}

TEST(MakeProductTest, SimpleProduct){
    Expression A = Expression('a', 4, 1);
    Expression B = Expression('s', 1, 0);

    Expression Prod_AB = Expression(4) * A * B;

    ASSERT_NE(Prod_AB.node_ptr, nullptr);
    auto node = Prod_AB.node_ptr;
    
    EXPECT_EQ(node->optype, OpType::Product);
    EXPECT_DOUBLE_EQ(node->scalar, 4.0);
    EXPECT_EQ(node->children.size(), 2);

    EXPECT_EQ(node->children[0]->payload, A.node_ptr->payload);
    EXPECT_EQ(node->children[1]->payload, B.node_ptr->payload);
}

TEST(MakeProductTest, NullProducts){
    Expression A = Expression('a', 4, 1);
    Expression O = Expression('a', 1, 1);

    Expression Prod_0 = O * A;

    ASSERT_NE(Prod_0.node_ptr, nullptr);
    auto node = Prod_0.node_ptr;
    
    EXPECT_EQ(node->optype, OpType::Scalar);
    EXPECT_DOUBLE_EQ(node->scalar, 0.0);
    EXPECT_EQ(node->children.size(), 0);
}

TEST(MakeProductTest, ProductDistribution){
    Expression A = Expression('a', 4, 1);
    Expression B = Expression('s', 1, 0);
    Expression C = Expression('s', 2, 3);

    Expression Prod_ABC = Expression(2) * A * (Expression(3) * B + C);

    ASSERT_NE(Prod_ABC.node_ptr, nullptr);
    auto node = Prod_ABC.node_ptr;
    
    EXPECT_EQ(node->optype, OpType::Sum);
    EXPECT_DOUBLE_EQ(node->scalar, 1.0);
    EXPECT_EQ(node->children.size(), 2);

    EXPECT_EQ(node->children[0]->children.size(), 2);
    EXPECT_EQ(node->children[0]->optype, OpType::Product);
    EXPECT_DOUBLE_EQ(node->children[0]->scalar, 6.0);
    EXPECT_EQ(node->children[0]->children[0]->payload, A.node_ptr->payload);
    EXPECT_EQ(node->children[0]->children[1]->payload, B.node_ptr->payload);

    EXPECT_EQ(node->children[1]->children.size(), 2);
    EXPECT_EQ(node->children[1]->optype, OpType::Product);
    EXPECT_DOUBLE_EQ(node->children[1]->scalar, -2.0);
    EXPECT_EQ(node->children[1]->children[0]->payload, A.node_ptr->payload);
    EXPECT_EQ(node->children[1]->children[1]->payload, C.node_ptr->payload);
}

TEST(MakeProductTest, MultipleSums){
    Expression A = Expression('a', 4, 1);
    Expression B = Expression('s', 0, 1);
    Expression C = Expression('s', 2, 3);
    Expression D = Expression('a', 0, 3);

    Expression Prod_ABCD = (A + B) * (C + D);

    ASSERT_NE(Prod_ABCD.node_ptr, nullptr);
    auto node = Prod_ABCD.node_ptr;
    
    EXPECT_EQ(node->optype, OpType::Sum);
    EXPECT_DOUBLE_EQ(node->scalar, 1.0);
    EXPECT_EQ(node->children.size(), 4);

    EXPECT_EQ(node->children[0]->children.size(), 2);
    EXPECT_EQ(node->children[0]->optype, OpType::Product);
    EXPECT_DOUBLE_EQ(node->children[0]->scalar, -1.0);
    EXPECT_EQ(node->children[0]->children[0]->payload, D.node_ptr->payload);
    EXPECT_EQ(node->children[0]->children[1]->payload, A.node_ptr->payload);

    EXPECT_EQ(node->children[3]->children.size(), 2);
    EXPECT_EQ(node->children[3]->optype, OpType::Product);
    EXPECT_DOUBLE_EQ(node->children[3]->scalar, 1.0);
    EXPECT_EQ(node->children[3]->children[0]->payload, B.node_ptr->payload);
    EXPECT_EQ(node->children[3]->children[1]->payload, C.node_ptr->payload);
}
