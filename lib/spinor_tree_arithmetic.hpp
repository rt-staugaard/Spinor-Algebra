#include <iostream>
#include <vector>
#include <memory>
#include <variant>
#include <algorithm>
#include <numeric>
#include <cassert>
#include <stdexcept>

struct Spinor {
    char type; // 'a' for angle <ij>, 's' for square [ij]
    int i;
    int j;

    Spinor(char type, int i, int j) : type(type), i(i), j(j) {}

    bool operator<(const Spinor& other) const {
        if (type != other.type) return type < other.type;
        if (i != other.i) return i < other.i;
        return j < other.j;
    }
    bool operator==(const Spinor& other) const {
        return type == other.type && i == other.i && j == other.j;
    }
    bool operator!=(const Spinor& other) const{
        return !(*this == other);
    }
};

enum class OpType {Scalar, Single, Sum, Product, Fraction};

struct Node;
using NodePtr = std::shared_ptr<const Node>;

struct Node {
    OpType optype;
    double scalar = 1.0; 
    std::variant<std::monostate, Spinor> payload;
    std::vector<NodePtr> children;

    bool operator<(const Node& other) const {
        if (optype != other.optype) return optype < other.optype;
        if (payload != other.payload) return payload < other.payload;
        if (children.size() != other.children.size()) return children.size() < other.children.size();

        for (int k = 0; k < children.size(); ++k){
            if (*children[k] != *other.children[k]){
                return *children[k] < *other.children[k];
            }
        }
        return false;
    }

    bool operator==(const Node& other) const {
        if (optype != other.optype || payload != other.payload) return false;
        if (children.size() != other.children.size()) return false;

        for (int k = 0; k < children.size(); ++k){
            if (*children[k] != *other.children[k]) return false;
        }
        return true;
    }

    bool operator!=(const Node& other) const {
        return !(*this == other);
    }
};

class Expression {
public:
    NodePtr node_ptr;
    Expression(NodePtr n) : node_ptr(std::move(n)) {}

    Expression(double val){
        auto scalar_node = std::make_shared<Node>();
        scalar_node->optype = OpType::Scalar;
        scalar_node->scalar = val;

        node_ptr = scalar_node;
    }

    Expression(char type, int i, int j){
        auto new_node = std::make_shared<Node>();
        if (i == j) {
            new_node->optype = OpType::Scalar;
            new_node->scalar = 0.0;
            node_ptr = std::move(new_node);
        }
        else {
            new_node->optype = OpType::Single;
            Spinor spinor(type, i, j);
            if (j > i){
                new_node->scalar *= -1;
                spinor.i = j;
                spinor.j = i;
            }
            new_node->payload = spinor;
            node_ptr = std::move(new_node);
        }
    }

    bool operator==(const Expression& other){
        return (*node_ptr == *other.node_ptr);
    }

    Expression operator+(const Expression& other) const;
    Expression operator-(const Expression& other) const;
    Expression operator*(const Expression& other) const;
    Expression operator/(const Expression& other) const;

    void print(std::ostream& os) const; 
};

static Expression singlet(char type, int i, int j){
    if (i == j) return Expression(0.0);

    double factor = 1.0;
    if (i > j){
        factor *= -1.0;
        std::swap(i,j);
    }

    auto singlet_node = std::make_shared<Node>();
    singlet_node->optype = OpType::Single;
    singlet_node->scalar = factor;
    singlet_node->payload = Spinor(type, i, j);

    return Expression(singlet_node);
}

static Expression make_sum(std::vector<Expression> terms){
    auto sum_node = std::make_shared<Node>();
    sum_node->optype = OpType::Sum;
    std::vector<NodePtr> flat_nodes;

    for (auto& t : terms){
        auto node = t.node_ptr;
        if (!node || (node->optype == OpType::Scalar && node->scalar == 0.0)) continue;

        if (node->optype == OpType::Sum){

            for (auto &child : node->children){
                auto scaled_child = std::make_shared<Node>(*child);
                scaled_child->scalar *= node->scalar;
                flat_nodes.push_back(scaled_child);
            }
            
        }
        else{
            flat_nodes.push_back(node);
        }
    }

    if (flat_nodes.empty()) return Expression(0.0);

    std::sort(flat_nodes.begin(), flat_nodes.end(), 
        [](const NodePtr& a, const NodePtr& b) {
            if (*a != *b) {
                return *a < *b;
            }
            return a->scalar < b->scalar;
        }
    );

    std::vector<NodePtr> adders;
    double scalar = 0.0;
    for (int i = 0; i < flat_nodes.size(); ++i){
        auto node = flat_nodes[i];

        if (i + 1 < flat_nodes.size()){
            auto next_node = flat_nodes[i + 1];

            if (*node == *next_node){
                scalar += node->scalar;
                continue;
            }
        }

        auto new_node = std::make_shared<Node>(*node);
        new_node->scalar += scalar;

        if (new_node->scalar != 0.0){
            adders.push_back(new_node);
        }
        scalar = 0.0;
    } 
    
    if (adders.empty()) return Expression(0.0);

    sum_node->children = adders;

    if (sum_node->children.size() == 1){
        auto new_node = std::make_shared<Node>();
        new_node->optype = sum_node->children[0]->optype;
        new_node->scalar = sum_node->children[0]->scalar;
        new_node->payload = sum_node->children[0]->payload;
        new_node->children = sum_node->children[0]->children;

        return Expression(new_node);        
    }

    return Expression(sum_node);
}

static Expression make_product(std::vector<Expression> terms){
    if (terms.size() == 0) return Expression(0.0);

    for (size_t i = 0; i < terms.size(); ++i){
        if (!terms[i].node_ptr) return Expression(0.0);

        if (terms[i].node_ptr->optype == OpType::Sum){
            auto sum_node = terms[i].node_ptr;
            std::vector<Expression> expanded_terms;

            for(auto& child : sum_node->children){
                std::vector<Expression> sub_factors;
                for (size_t j = 0; j < terms.size(); ++j){
                    if (i == j){
                        sub_factors.push_back(Expression(child));
                    }
                    else{
                        sub_factors.push_back(terms[j]);
                    }
                }
                expanded_terms.push_back(make_product(sub_factors));
            }

            return make_sum(expanded_terms);
        }
    }

    std::vector<NodePtr> products;
    
    double total_factor = 1.0; 
    for(auto &t : terms){
        auto node = t.node_ptr;
        total_factor *= t.node_ptr->scalar;
        
        if (total_factor == 0.0) return Expression(0.0);

        if (node->optype == OpType::Product){
            for (auto &child : node->children){
                auto unit_child = std::make_shared<Node>(*child);
                unit_child->scalar = 1.0;
                products.push_back(unit_child);
            }
        }

        else if(node->optype != OpType::Scalar){
            auto unit_node = std::make_shared<Node>(*node);
            unit_node->scalar = 1.0;
            products.push_back(unit_node); 
        }
    }

    if (products.empty()) {
        return Expression(total_factor);
    }

    std::sort(products.begin(), products.end(), 
        [](const NodePtr& a, const NodePtr& b) {
            if (*a != *b) {
                return *a < *b;
            }
            return a->scalar < b->scalar;
        }
    );

    if (products.size() == 1) {
        auto result_node = std::make_shared<Node>(*products[0]);
        result_node->scalar *= total_factor;
        return Expression(result_node);
    }

    auto product_node = std::make_shared<Node>();
    product_node->optype = OpType::Product;
    product_node->scalar = total_factor;
    product_node->children = products;

    return Expression(product_node);
}

static Expression make_fraction(Expression numerator, Expression denominator){

    if(denominator.node_ptr->optype == OpType::Scalar && denominator.node_ptr->scalar == 0.0) {
        throw std::invalid_argument("Undefined behaviour: Cannot divide by zero.");
    }
    if (numerator.node_ptr->optype == OpType::Scalar && numerator.node_ptr->scalar == 0.0) return Expression(0.0);

    auto num_node = numerator.node_ptr;
    auto den_node = denominator.node_ptr;

    if (*num_node == *den_node) return Expression(1.0);

    if (num_node->optype == OpType::Fraction){
        Expression new_den = make_product({Expression(num_node->children[1]), denominator});
        return make_fraction(Expression(num_node->children[0]), new_den);
    }

    if (den_node->optype == OpType::Fraction){
        Expression new_num = make_product({numerator, Expression(den_node->children[1])});
        return make_fraction(new_num, denominator);
    }

    auto frac_node = std::make_shared<Node>();
    frac_node->optype = OpType::Fraction;
    frac_node->scalar = num_node->scalar / den_node->scalar;

    auto n_num = std::make_shared<Node>(*num_node); 
    n_num->scalar = 1.0;

    auto n_den = std::make_shared<Node>(*den_node); 
    n_den->scalar = 1.0;

    frac_node->children = {n_num, n_den};

    return Expression(frac_node);
}

Expression Expression::operator+(const Expression& other) const {
    return make_sum({*this, other});
}

Expression Expression::operator-(const Expression& other) const {
    auto neg_node = std::make_shared<Node>();
    neg_node->optype = other.node_ptr->optype;
    neg_node->scalar = -other.node_ptr->scalar;
    neg_node->payload = other.node_ptr->payload;
    neg_node->children = other.node_ptr->children;
    return make_sum({*this, Expression(neg_node)});
}

Expression Expression::operator*(const Expression& other) const {
    return make_product({*this, other});
}

Expression Expression::operator/(const Expression& other) const {
    return make_fraction(*this, other);
}

void Expression::print(std::ostream& os) const {
    if(!node_ptr){
        os << "null";
        return;
    }

    if (node_ptr->scalar != 1.0 && node_ptr->optype != OpType::Scalar){
        os << node_ptr->scalar << "*";
    }

    switch (node_ptr->optype){

        case OpType::Scalar:{
            os << node_ptr->scalar;
            break;
        }
        
        case OpType::Single:{
            auto spinor = std::get<Spinor>(node_ptr->payload);
            char open = (spinor.type == 'a') ? '<' : '[';
            char close = (spinor.type == 'a') ? '>' : ']'; 
            os << open << spinor.i << " " << spinor.j << close;
            break;
        }

        case OpType::Product:{
            for (auto& c : node_ptr->children){
                Expression(c).print(os);
            }
            break;
        }

        case OpType::Sum:{
            os << "(";
            for (size_t i = 0; i < node_ptr->children.size(); ++i){
                if (i > 0) {
                    os << " + " ;
                }
                Expression(node_ptr->children[i]).print(os);
            }
            os << ")";
            break;
        }

        case OpType::Fraction:{
            os << "(";
            Expression(node_ptr->children[0]).print(os);
            os << ") / (";
            Expression(node_ptr->children[1]).print(os);
            os << ")";
            break;
        }
    }
} 

std::ostream& operator<<(std::ostream& os, const Expression& exs){
    exs.print(os);
    return os;
} 