#include <gtest/gtest.h>
#include "../lib/spinor_tree_arithmetic.hpp"

TEST(MakeSumTest, IdenticalSum){
    Expression A = Expression('a', 1, 3);

    Expression Sum_2A = A + A;

    ASSERT_NE(Sum_2A.node_ptr, nullptr);
    auto node = Sum_2A.node_ptr;
    
    EXPECT_EQ(node->optype, OpType::Single);
    EXPECT_DOUBLE_EQ(node->scalar, 2.0);
    EXPECT_EQ(node->payload, A.node_ptr->payload);
    EXPECT_EQ(node->children.size(), 0);
}

TEST(MakeSumTest, Cancellation){
    Expression A = Expression('a', 1, 3);

    Expression Sum_0 = A - A;

    ASSERT_NE(Sum_0.node_ptr, nullptr);
    auto node = Sum_0.node_ptr;
    
    EXPECT_EQ(node->optype, OpType::Scalar);
    EXPECT_DOUBLE_EQ(node->scalar, 0.0);
    EXPECT_TRUE(std::holds_alternative<std::monostate>(node->payload));
    EXPECT_EQ(node->children.size(), 0);
}

TEST(MakeSumTest, BasicSum){
    Expression A = Expression('a', 1, 3);
    Expression B = Expression('a', 2, 4);

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
    Expression A = Expression('a', 1, 3);
    Expression C = Expression('s', 3, 2);

    Expression Sum_ACA = A + Expression(2) * C + Expression(3) * A;

    ASSERT_NE(Sum_ACA.node_ptr, nullptr);
    auto node = Sum_ACA.node_ptr;
    
    EXPECT_EQ(node->optype, OpType::Sum);
    EXPECT_DOUBLE_EQ(node->scalar, 1.0);
    EXPECT_TRUE(std::holds_alternative<std::monostate>(node->payload));
    EXPECT_EQ(node->children.size(), 2);
    EXPECT_EQ(node->children[0]->payload, A.node_ptr->payload);
    EXPECT_EQ(node->children[0]->scalar, 4.0);
    EXPECT_EQ(node->children[1]->payload, C.node_ptr->payload);
    EXPECT_EQ(node->children[1]->scalar, -2.0);
}

TEST(MakeProductTest, SimpleProduct){
    Expression A = Expression('a', 1, 4);
    Expression B = Expression('s', 0, 1);

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
    Expression A = Expression('a', 1, 4);
    Expression O = Expression('a', 1, 1);

    Expression Prod_0 = O * A;

    ASSERT_NE(Prod_0.node_ptr, nullptr);
    auto node = Prod_0.node_ptr;
    
    EXPECT_EQ(node->optype, OpType::Scalar);
    EXPECT_DOUBLE_EQ(node->scalar, 0.0);
    EXPECT_EQ(node->children.size(), 0);
}

TEST(MakeProductTest, ProductDistribution){
    Expression A = Expression('a', 1, 4);
    Expression B = Expression('s', 0, 1);
    Expression C = Expression('s', 3, 2);

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
    Expression A = Expression('a', 1, 4);
    Expression B = Expression('s', 1, 0);
    Expression C = Expression('s', 3, 2);
    Expression D = Expression('a', 3, 0);

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

TEST(MakeFractionTest, TrivialFraction){
    Expression A = Expression('s', 2, 3);

    Expression Frac_1 = A / A;

    ASSERT_NE(Frac_1.node_ptr, nullptr);
    auto node = Frac_1.node_ptr;

    EXPECT_EQ(node->optype, OpType::Scalar);
    EXPECT_DOUBLE_EQ(node->scalar, 1.0);
    EXPECT_TRUE(std::holds_alternative<std::monostate>(node->payload));
    EXPECT_EQ(node->children.size(), 0);
}

TEST(MakeFractionTest, SimpleFraction){
    Expression A = Expression('s', 2, 3);
    Expression B = Expression('a', 0, 3);

    Expression Frac_AB = A / B;

    ASSERT_NE(Frac_AB.node_ptr, nullptr);
    auto node = Frac_AB.node_ptr;

    EXPECT_EQ(node->optype, OpType::Fraction);
    EXPECT_DOUBLE_EQ(node->scalar, 1.0);
    EXPECT_TRUE(std::holds_alternative<std::monostate>(node->payload));
    EXPECT_EQ(node->children.size(), 2);
    EXPECT_TRUE(is_equal(node->children[0], A.node_ptr));
    EXPECT_TRUE(is_equal(node->children[1], B.node_ptr));
}


TEST(MakeFractionTest, SumFraction){
    Expression A = Expression('s', 2, 3);
    Expression B = Expression('a', 0, 3);
    Expression C = Expression('a', 1, 5);

    Expression Frac_ABAC = (Expression(3) * A + B) / (Expression(2) * A + Expression(2) * C);

    ASSERT_NE(Frac_ABAC.node_ptr, nullptr);
    auto node = Frac_ABAC.node_ptr;

    EXPECT_EQ(node->optype, OpType::Fraction);
    EXPECT_DOUBLE_EQ(node->scalar, 1.0);
    EXPECT_TRUE(std::holds_alternative<std::monostate>(node->payload));
    EXPECT_EQ(node->children.size(), 2);
    EXPECT_EQ(node->children[0]->optype, OpType::Sum);
    EXPECT_EQ(node->children[1]->optype, OpType::Sum);

    EXPECT_TRUE(is_equal(node->children[0]->children[0], (B).node_ptr));
    EXPECT_TRUE(is_equal(node->children[0]->children[1], (Expression(3) * A.node_ptr).node_ptr));
    EXPECT_TRUE(is_equal(node->children[1]->children[0], (Expression(2) * C).node_ptr));
    EXPECT_TRUE(is_equal(node->children[1]->children[1], (Expression(2) * A).node_ptr));
}

TEST(MakeFractionTest, FractionWithSum){
    Expression A = Expression('s', 2, 3);
    Expression B = Expression('a', 0, 3);
    Expression C = Expression('a', 1, 5);
    Expression D = Expression('s', 2, 4);

    Expression Frac_ABCD = B / (A + C) +  D;

    ASSERT_NE(Frac_ABCD.node_ptr, nullptr);
    auto node = Frac_ABCD.node_ptr;

    EXPECT_EQ(node->optype, OpType::Fraction);
    EXPECT_DOUBLE_EQ(node->scalar, 1.0);
    EXPECT_TRUE(std::holds_alternative<std::monostate>(node->payload));
    EXPECT_EQ(node->children.size(), 2);
    EXPECT_EQ(node->children[0]->optype, OpType::Sum);
    EXPECT_EQ(node->children[1]->optype, OpType::Sum);

    EXPECT_EQ(node->children[0]->children.size(), 3);
    EXPECT_EQ(node->children[1]->children.size(), 2);

    EXPECT_TRUE(is_equal(node->children[1]->children[0], C.node_ptr));
    EXPECT_TRUE(is_equal(node->children[1]->children[1], A.node_ptr));

    EXPECT_EQ(node->children[0]->children[0]->optype, OpType::Single);
    EXPECT_TRUE(is_equal(node->children[0]->children[0], B.node_ptr));

    EXPECT_EQ(node->children[0]->children[1]->optype, OpType::Product);
    EXPECT_TRUE(is_equal(node->children[0]->children[1]->children[0], C.node_ptr));
    EXPECT_TRUE(is_equal(node->children[0]->children[1]->children[1], D.node_ptr));

    EXPECT_EQ(node->children[0]->children[2]->optype, OpType::Product);
    EXPECT_TRUE(is_equal(node->children[0]->children[2]->children[0], A.node_ptr));
    EXPECT_TRUE(is_equal(node->children[0]->children[2]->children[1], D.node_ptr));
}


TEST(MakeFractionTest, SumOfFraction){
    Expression A = Expression('s', 2, 3);
    Expression B = Expression('a', 0, 3);
    Expression C = Expression('a', 1, 5);
    Expression D = Expression('s', 2, 4);

    Expression Frac_ABCD = B / (A + C) +  D / (A + C);

    ASSERT_NE(Frac_ABCD.node_ptr, nullptr);
    auto node = Frac_ABCD.node_ptr;

    EXPECT_EQ(node->optype, OpType::Fraction);
    EXPECT_DOUBLE_EQ(node->scalar, 1.0);
    EXPECT_TRUE(std::holds_alternative<std::monostate>(node->payload));
    EXPECT_EQ(node->children.size(), 2);
    EXPECT_EQ(node->children[0]->optype, OpType::Sum);
    EXPECT_EQ(node->children[1]->optype, OpType::Sum);

    EXPECT_EQ(node->children[0]->children.size(), 2);
    EXPECT_EQ(node->children[1]->children.size(), 2);

    EXPECT_TRUE(is_equal(node->children[1]->children[0], C.node_ptr));
    EXPECT_TRUE(is_equal(node->children[1]->children[1], A.node_ptr));

    EXPECT_TRUE(is_equal(node->children[0]->children[0], B.node_ptr));
    EXPECT_TRUE(is_equal(node->children[0]->children[1], D.node_ptr));
}


TEST(MakeFractionTest, FractionCollection){
    Expression A = Expression('s', 2, 3);
    Expression B = Expression('a', 0, 3);
    Expression C = Expression('a', 1, 5);

    Expression Frac_1 = B / (A + C);
    Expression Frac_2 =  (Expression(2) * B) / (A + C);
    Expression Frac_3 = (Expression(5) * B) / (A + C);

    std::vector<NodePtr> nodes = {Frac_1.node_ptr, Frac_2.node_ptr, Frac_3.node_ptr};
    std::vector<NodePtr> adders;

    collect_fraction_sums(nodes, adders);

    EXPECT_EQ(adders.size(), 1);
    EXPECT_EQ(adders[0]->scalar, 1.0);
    EXPECT_EQ(adders[0]->optype, OpType::Fraction);
    EXPECT_EQ(adders[0]->children[0]->scalar, 8.0);
    EXPECT_EQ(*adders[0]->children[0], *B.node_ptr);
    EXPECT_EQ(*adders[0]->children[1]->children[0], *C.node_ptr);
    EXPECT_EQ(*adders[0]->children[1]->children[1], *A.node_ptr);
}

TEST(MakeFractionTest, DoubleFraction){
    Expression A = Expression('s', 2, 3);
    Expression B = Expression('a', 0, 3);
    Expression C = Expression('a', 1, 5);
    Expression D = Expression('s', 2, 4);
    Expression E = Expression('a', 1, 3);

    Expression Frac_ABCDE = (A / (B + C) + B / (B + C)) / D;

    ASSERT_NE(Frac_ABCDE.node_ptr, nullptr);
    auto node = Frac_ABCDE.node_ptr;

    EXPECT_EQ(node->optype, OpType::Fraction);
    EXPECT_DOUBLE_EQ(node->scalar, 1.0);
    EXPECT_TRUE(std::holds_alternative<std::monostate>(node->payload));
    EXPECT_EQ(node->children.size(), 2);
    EXPECT_EQ(node->children[0]->optype, OpType::Sum);
    EXPECT_EQ(node->children[1]->optype, OpType::Sum);

    EXPECT_EQ(node->children[0]->children.size(), 2);
    EXPECT_EQ(node->children[1]->children.size(), 2);

    EXPECT_TRUE(is_equal(node->children[0]->children[0], B.node_ptr));
    EXPECT_TRUE(is_equal(node->children[0]->children[1], A.node_ptr));

    EXPECT_TRUE(is_equal(node->children[1]->children[0], (B * D).node_ptr));
    EXPECT_TRUE(is_equal(node->children[1]->children[1], (C * D).node_ptr));
}

TEST(MakeFractionTest, MassiveReduction){
    Expression A = Expression('s', 2, 3);
    Expression B = Expression('a', 0, 3);
    Expression C = Expression('a', 1, 5);
    Expression D = Expression('s', 2, 4);

    Expression Frac_ABCD = ((Expression(3) * A) * (Expression(4) * B) * (Expression(2) * C) * (D)) / (Expression(3) * A * B);

    ASSERT_NE(Frac_ABCD.node_ptr, nullptr);
    auto node = Frac_ABCD.node_ptr;

    EXPECT_EQ(node->optype, OpType::Product);
    EXPECT_DOUBLE_EQ(node->scalar, 8.0);
    EXPECT_TRUE(std::holds_alternative<std::monostate>(node->payload));
    EXPECT_EQ(node->children.size(), 2);

    EXPECT_EQ(*node->children[0], *(C).node_ptr);
    EXPECT_EQ(*node->children[1], *(D).node_ptr);
}

TEST(EdgeCases, AdditiveIdentityAndZeroPruning){
    Expression A = Expression('a', 1, 3);
    Expression Zero = Expression(0.0);

    Expression Sum_A0 = A + Zero;
    EXPECT_EQ(Sum_A0.node_ptr->optype, OpType::Single);
    EXPECT_DOUBLE_EQ(Sum_A0.node_ptr->scalar, A.node_ptr->scalar);

    Expression Sum_ComplexZero = A + (Zero * Expression('b', 1, 2));
    EXPECT_EQ(Sum_ComplexZero.node_ptr->optype, OpType::Single);
}

// Not Implemented Yet
//TEST(EdgeCases, CompleteSumCancellation){
//    Expression A = Expression('s', 2, 3);
//    Expression B = Expression('a', 0, 3);
//    Expression C = Expression('a', 1, 5);

//    Expression Frac_ABC = ((A + B) * C) / (A + B);

//    ASSERT_NE(Frac_ABC.node_ptr, nullptr);
//    auto node = Frac_ABC.node_ptr;

//    EXPECT_TRUE(is_equal(node, C.node_ptr));
//}

TEST(EdgeCases, FractionOverFraction){
    Expression A = Expression('s', 2, 3);
    Expression B = Expression('a', 0, 3);
    Expression C = Expression('a', 1, 5);
    Expression D = Expression('s', 2, 4);

    Expression Frac = (A / B) / (C / D);

    ASSERT_NE(Frac.node_ptr, nullptr);
    auto node = Frac.node_ptr;

    EXPECT_EQ(node->optype, OpType::Fraction);
    EXPECT_TRUE(is_equal(node->children[0], (A * D).node_ptr));
    EXPECT_TRUE(is_equal(node->children[1], (B * C).node_ptr));

}

TEST(EdgeCases, ComplexSumCancellationToZero){
    Expression A = Expression('a', 1, 3);
    Expression B = Expression('s', 2, 4);

    Expression Sum_Zero = (Expression(3) * A + Expression(2) * B) - (Expression(3) * A + Expression(2) * B);

    ASSERT_NE(Sum_Zero.node_ptr, nullptr);
    EXPECT_EQ(Sum_Zero.node_ptr->optype, OpType::Scalar);
    EXPECT_DOUBLE_EQ(Sum_Zero.node_ptr->scalar, 0.0);
}