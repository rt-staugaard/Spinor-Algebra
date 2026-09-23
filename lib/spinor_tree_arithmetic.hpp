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

    // COMPARISION WITHOUT SCALAR
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

// COMPARISON WITH SCALAR
bool is_equal(NodePtr node1, NodePtr node2){
    if (node1->optype != node2->optype || node1->payload != node2->payload) return false;
    if (node1->children.size() != node2->children.size()) return false;

    for (int k = 0; k < node1->children.size(); ++k){
        if (*node1->children[k] != *node2->children[k] || node1->children[k]->scalar != node2->children[k]->scalar) return false;
    }
    return true;
}

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
            if (i > j){
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

static Expression make_sum(std::vector<Expression> terms);
static Expression make_product(std::vector<Expression> terms);
static Expression make_fraction(Expression numerator, Expression denominator);
Expression distribute_sum(Expression ex1, Expression ex2);

Expression single_node_to_expression(const NodePtr& node, double scalar = 1.0){
    auto new_node = std::make_shared<Node>();
    new_node->optype = node->optype;
    new_node->scalar = scalar * node->scalar;
    new_node->payload = node->payload;
    new_node->children = node->children;

    return Expression(new_node);
}

Expression vector_node_to_sum(const std::vector<NodePtr>& children, double scalar = 1.0){
    if (children.size() == 0) return Expression(0.0);
    
    if (children.size() == 1){
        return single_node_to_expression(children[0], scalar);
    }
    
    auto new_node = std::make_shared<Node>();
    new_node->optype = OpType::Sum;
    new_node->scalar = scalar;
    new_node->children = children;

    return Expression(new_node);
}

Expression vector_node_to_product(const std::vector<NodePtr>& children, double scalar = 1.0){
    if (children.size() == 0) return Expression(0.0);

    if (children.size() == 1){
        return single_node_to_expression(children[0], scalar);
    }

    auto new_node = std::make_shared<Node>();
    new_node->optype = OpType::Product;
    new_node->scalar = scalar;
    new_node->children = children;

    return Expression(new_node);
}

Expression nodes_to_fraction(const NodePtr num, const NodePtr den,  double scalar = 1.0){
    Expression numerator = single_node_to_expression(num, scalar);
    Expression denominator = single_node_to_expression(den);

    auto new_node = std::make_shared<Node>();
    new_node->optype = OpType::Fraction;
    new_node->scalar = scalar;
    new_node->children = {numerator.node_ptr, denominator.node_ptr};

    return Expression(new_node);
}

NodePtr normalize(const NodePtr node){
    auto new_node = std::make_shared<Node>();
    new_node->optype = node->optype;
    new_node->scalar = 1.0;
    new_node->payload = node->payload;
    new_node->children = node->children;

    return new_node;
}


void collect_sums(std::vector<NodePtr> &nodes, std::vector<NodePtr> &adders){

    std::sort(nodes.begin(), nodes.end(), 
            [](const NodePtr& a, const NodePtr& b) {
                return *a < *b;
        }
    );

    double scalar = 0.0;
    for (int i = 0; i < nodes.size(); ++i){
        auto node = nodes[i];

        if (i + 1 < nodes.size()){
            auto next_node = nodes[i + 1];

            if (*node == *next_node){
                scalar += node->scalar;
                continue;
            }
        }

        double total_scalar = scalar + node->scalar;
        if (total_scalar != 0.0) {
            auto unit_node = normalize(node);
            auto new_node = single_node_to_expression(unit_node, total_scalar).node_ptr;
            adders.push_back(new_node);
        }
        scalar = 0.0;
    } 
}

void collect_fraction_sums(std::vector<NodePtr> &nodes, std::vector<NodePtr> &adders){
    if (nodes.empty()) return;

    std::sort(nodes.begin(), nodes.end(), 
            [](const NodePtr& a, const NodePtr& b) {
                return *a->children[1] < *b->children[1];
        }
    );

    std::vector<NodePtr> same_denominator;
    for (int i = 0; i < nodes.size(); ++i){
        auto denominator = nodes[i]->children[1];

        if (i + 1 < nodes.size()){
            auto next_denominator = nodes[i + 1]->children[1];

            if (*denominator == *next_denominator){
                auto numerator = single_node_to_expression(nodes[i]->children[0], 1.0);
                same_denominator.push_back(numerator.node_ptr);
                continue;
            }
        }

        if (!same_denominator.empty()){
            auto numerator = single_node_to_expression(nodes[i]->children[0], 1.0);
            same_denominator.push_back(numerator.node_ptr);

            std::vector<NodePtr> simplified;
            collect_sums(same_denominator, simplified);
            if (!simplified.empty()){
                auto num_node = vector_node_to_sum(simplified).node_ptr;
                auto fraction = nodes_to_fraction(num_node, denominator);
                adders.push_back(fraction.node_ptr);
            }
            same_denominator.clear();
        }
        else{
            adders.push_back(nodes[i]);
        }
    }
}

static Expression make_sum(std::vector<Expression> terms){
    if (terms.size() == 0) return Expression(0.0);

    std::vector<NodePtr> flat_nodes;
    std::vector<NodePtr> frac_nodes;

    for (size_t i = 0; i < terms.size(); ++i){
        auto node = terms[i].node_ptr;
        if (!node || node->scalar == 0.0) continue;

        if (node->optype == OpType::Sum){

            for (auto &child : node->children){
                auto scaled_child = std::make_shared<Node>(*child);
                scaled_child->scalar *= node->scalar;
                flat_nodes.push_back(scaled_child);
            }
        }
        else if (node->optype == OpType::Fraction){
            frac_nodes.push_back(node);
        }
        else {
            flat_nodes.push_back(node);
        }
    }

    std::vector<NodePtr> adders;
    if (!flat_nodes.empty()){
        collect_sums(flat_nodes, adders);
    }
    
    std::vector<NodePtr> frac_adders;
    if (!frac_nodes.empty()){
        collect_fraction_sums(frac_nodes, frac_adders);
    }
    else {
        return vector_node_to_sum(adders);
    }

    Expression numerator = vector_node_to_sum(adders);
    Expression denominator = Expression(1.0);
    for (size_t i = 0; i < frac_adders.size(); ++i){
        numerator = distribute_sum(Expression(frac_adders[i]->children[1]), numerator);
        numerator = make_sum({Expression(frac_adders[i]->children[0]), numerator});
        denominator = make_product({Expression(frac_adders[i]->children[1]), denominator});
    }

    return make_fraction(numerator, denominator); 
}

Expression distribute_sum(Expression ex1, Expression ex2){
    auto n1 = ex1.node_ptr;
    auto n2 = ex2.node_ptr;

    if (!n1 || !n2) return Expression(0.0);

    if (n1->optype == OpType::Sum && n2->optype == OpType::Sum) {
        std::vector<Expression> combined_terms;
        for (auto& child1 : n1->children) {
            for (auto& child2 : n2->children) {
                combined_terms.push_back(make_product({Expression(child1), Expression(child2)}));
            }
        }
        return make_sum(combined_terms);
    }

    if (n1->optype == OpType::Sum) {
        std::vector<Expression> combined_terms;
        for (auto& child1 : n1->children) {
            combined_terms.push_back(make_product({Expression(child1), ex2}));
        }
        return make_sum(combined_terms);
    }

    if (n2->optype == OpType::Sum) {
        return distribute_sum(ex2, ex1);
    }

    return make_product({ex1, ex2});
}


void reduce_factors(std::vector<NodePtr>& num_list, std::vector<NodePtr>& den_list) {
    if (num_list.size() == 0) return;

    for (auto it_num = num_list.begin(); it_num != num_list.end(); ) {
        bool erased = false;
        
        for (auto it_den = den_list.begin(); it_den != den_list.end(); ++it_den) {
            if (is_equal(*it_num, *it_den)) {
                
                it_num = num_list.erase(it_num);
                
                den_list.erase(it_den);
                
                erased = true;
                break; 
            }
        }
        
        if (!erased) {
            ++it_num;
        }
    }
}

void extract_factors(const NodePtr& node, std::vector<NodePtr>& factors) {
    if (!node || node->optype == OpType::Scalar) return;

    if (node->optype == OpType::Product) {
        for (const auto& child : node->children) {
            extract_factors(child, factors); 
        }
    } 
    else {
        auto unit_factor = std::make_shared<Node>(*node);
        unit_factor->scalar = 1.0; 
        factors.push_back(unit_factor);
    }
}

void reduce_fraction(NodePtr& numerator, NodePtr& denominator){

    std::vector<NodePtr> num_factors;
    extract_factors(numerator, num_factors);

    std::vector<NodePtr> den_factors;
    extract_factors(denominator, den_factors);

    if (!num_factors.empty() && !den_factors.empty()){
        reduce_factors(num_factors, den_factors);
    }

    Expression new_num = vector_node_to_product(num_factors, numerator->scalar);
    Expression new_den = vector_node_to_product(den_factors, denominator->scalar);

    numerator = new_num.node_ptr;
    denominator = new_den.node_ptr;
}


static Expression make_product(std::vector<Expression> terms){
    if (terms.size() == 0) return Expression(0.0);

    std::vector<NodePtr> factors;
    std::vector<NodePtr> sum_factors;
    std::vector<NodePtr> division_factors;

    double total_factor = 1.0;
    for (size_t i = 0; i < terms.size(); ++i){
        auto node = terms[i].node_ptr;
        if (!node || node->scalar == 0.0) return Expression(0.0);
        
        total_factor *= node->scalar;
        OpType type = node->optype;
        switch (type) {
            case OpType::Scalar:{
                break;
            }

            case OpType::Single:{
                auto unit_node = normalize(node);
                factors.push_back(unit_node);
                break;
            }

            case OpType::Product:{
                for (auto &child : node->children){
                    total_factor *= child->scalar;
                    auto unit_child = normalize(child);
                    factors.push_back(unit_child);
                }
                break;
            }

            case OpType::Sum:{
                auto unit_sum = normalize(node);
                sum_factors.push_back(unit_sum);
                break;
            }

            case OpType::Fraction:{
                OpType num_type = node->children[0]->optype;
                auto unit_frac = normalize(node);

                if (num_type == OpType::Single){
                    factors.push_back(unit_frac->children[0]);
                }
                else if (num_type == OpType::Product){
                    for (auto& c : unit_frac->children[0]->children){
                        total_factor *= c->scalar;
                        factors.push_back(normalize(c));
                    }
                }
                else if (num_type == OpType::Sum){
                    sum_factors.push_back(unit_frac->children[0]);
                }

                division_factors.push_back(unit_frac->children[1]);
                break;
            }
        }
    }

    std::sort(factors.begin(), factors.end(), 
        [](const NodePtr& a, const NodePtr& b) {
            if (*a != *b) {
                return *a < *b;
            }
            return a->scalar < b->scalar;
        }
    );

    if (!division_factors.empty()){
        reduce_factors(factors, division_factors);
        reduce_factors(sum_factors, division_factors);
    }

    if (division_factors.empty() && sum_factors.empty()){
        return vector_node_to_product(factors, total_factor);
    }

    Expression numerator = Expression(total_factor);
    if (!factors.empty()){
        numerator = vector_node_to_product(factors, total_factor);
    }

    for (auto& s : sum_factors) {
        numerator = distribute_sum(numerator, Expression(s));
    }
    
    if(division_factors.empty()){
        return numerator;
    }
    
    auto denominator = vector_node_to_product(division_factors);

    return make_fraction(numerator, denominator);
}


static Expression make_fraction(Expression numerator, Expression denominator){

    if(denominator.node_ptr->optype == OpType::Scalar) {
        if (denominator.node_ptr->scalar == 0.0){
            throw std::invalid_argument("Undefined behaviour: Cannot divide by zero.");
        }
        else {
            auto node = std::make_shared<Node>();
            node->optype =numerator.node_ptr->optype;
            node->scalar = numerator.node_ptr->scalar / denominator.node_ptr->scalar;
            node->children = numerator.node_ptr->children;
            return Expression(node);
        }
    }
    if (numerator.node_ptr->optype == OpType::Scalar && numerator.node_ptr->scalar == 0.0) return Expression(0.0);

    auto num_node = numerator.node_ptr;
    auto den_node = denominator.node_ptr;

    if (*num_node == *den_node) return Expression(1.0);

    if (num_node->optype == OpType::Fraction) {
        Expression n1 = single_node_to_expression(num_node->children[0], num_node->scalar);
        Expression d1 = Expression(num_node->children[1]); 

        Expression new_den = make_product({d1, denominator});
        return make_fraction(n1, new_den);
    }

    if (den_node->optype == OpType::Fraction) {
        Expression n2 = single_node_to_expression(den_node->children[0], den_node->scalar);
        Expression d2 = Expression(den_node->children[1]); 

        Expression new_num = make_product({numerator, d2});
        return make_fraction(new_num, n2);
    }

    double overall_scalar = num_node->scalar / den_node->scalar;
    num_node = normalize(num_node);
    den_node = normalize(den_node);

    reduce_fraction(num_node, den_node);

    if (den_node->optype == OpType::Scalar) {
        return single_node_to_expression(num_node, overall_scalar);
    }

    return nodes_to_fraction(num_node, den_node, overall_scalar);
}



Expression Expression::operator+(const Expression& other) const {
    return make_sum({*this, other});
}

Expression Expression::operator-(const Expression& other) const {
    auto negative_expression = single_node_to_expression(other.node_ptr, -1.0);
    return make_sum({*this, negative_expression});
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