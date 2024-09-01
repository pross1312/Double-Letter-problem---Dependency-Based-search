#include <cstdio>
#include <string>
#include <cassert>
#include <vector>
#include <memory>

struct Node;
typedef std::shared_ptr<Node> NodePtr;
#define make_node std::make_shared<Node>
#define LOG_NODE(node) printf("[%s, (%zu, %zu, %c, %c), %zu, %zu]\n",\
    (node).type == Node::Type::Dependency ? "Dependency" : "Combination",\
    (node).op.start, (node).op.end, (node).op.prev, (node).op.cur,\
    (node).level,\
    (node).children.size())

std::pair<char, char> convert_double_letter(char c) {
    const int len = ('e' - 'a' + 1);
    return std::pair((char)((c - 'a' - 1 + len)%len + 'a'), (char)((c - 'a' + 1)%len + 'a'));
}

struct Node {
    enum Type {
        Dependency,
        Combination,
    } type;
    struct Operation {
        size_t start, end;
        char prev, cur;
        bool operator==(const Operation that) { return start == that.start && end == that.end && prev == that.prev && cur == that.cur; }
    } op;
    static constexpr Operation InvalidOperation {.start = (size_t)-2, .end = (size_t)-2, .prev = '\0', .cur = '\0'};
    size_t level;
    std::vector<NodePtr> children;


    Node(Node::Type type, Node::Operation op, size_t level): type(type), op(op), level(level) {}
};

struct DBSearcher {
    DBSearcher(std::string_view state):
        state(state), root(make_node(Node::Type::Combination, Node::InvalidOperation, 0)), tree_size_growed(true) {}

    NodePtr search() {
        size_t level = 1;
        while (tree_size_growed) {
            tree_size_growed = false;
            dependency_stage(root, level);
            combination_stage(level);
            level++;
        }
        return nullptr;
    }

    bool combinable(Node::Operation op1, Node::Operation op2) {
        return op1.cur == op2.cur && (op1.end + 1 == op2.start || op2.end + 1 == op1.start);
    }

    std::pair<NodePtr, NodePtr> combine(const NodePtr& node1, const NodePtr& node2, const size_t& level) {
        auto[char1, char2] = convert_double_letter(node1->op.cur);
        std::pair<NodePtr, NodePtr> result;
        result.first = make_node(Node::Type::Combination, Node::Operation{
            .start = std::min(node1->op.start, node2->op.start),
            .end = std::max(node1->op.end, node2->op.end),
            .prev = node1->op.cur,
            .cur = char1
        }, level);
        result.second = make_node(Node::Type::Combination, Node::Operation{
            .start = std::min(node1->op.start, node2->op.start),
            .end = std::max(node1->op.end, node2->op.end),
            .prev = node1->op.cur,
            .cur = char2
        }, level);
        return result;
    }

    void combination_stage(const size_t level) {
        assert(nodes.size() > 1);
        for (size_t i = nodes.size()-1; i >= 1; i--) {
            const NodePtr& node = nodes[i];
            if (node->type == Node::Type::Dependency && node->level + 1 == level) {
                for (size_t j = 0; j < i ; j++) {
                    const NodePtr& that = nodes[j];
                    if (that->type == Node::Type::Dependency && combinable(node->op, that->op)) {
                        auto[child1, child2] = combine(node, that, level);
                        LOG_NODE(*child1);
                        LOG_NODE(*child2);
                        nodes.push_back(child1);
                        nodes.push_back(child2);
                        node->children.push_back(child1);
                        node->children.push_back(child2);
                        tree_size_growed = true;
                    }
                }
            }
        }
    }

    bool dependency_stage(NodePtr node, const size_t& level) {
        bool tree_growed = false;
        if (node->level + 1 == level) {
            if (node->op == Node::InvalidOperation) {
                for (size_t i = 0; i < state.size()-1; i++) {
                    if (state[i] == state[i+1]) {
                        auto[char_1, char_2] = convert_double_letter(state[i]);
                        NodePtr child_1 = make_node(Node::Type::Dependency, Node::Operation{
                                .start = i,
                                .end = i+1,
                                .prev = state[i],
                                .cur = char_1,
                                }, node->level);
                        LOG_NODE(*child_1);
                        NodePtr child_2 = make_node(Node::Type::Dependency, Node::Operation{
                                .start = i,
                                .end = i+1,
                                .prev = state[i],
                                .cur = char_2,
                                }, node->level);
                        node->children.push_back(child_1);
                        node->children.push_back(child_2);
                        if (!dependency_stage(child_1, level)) nodes.push_back(child_1);
                        if (!dependency_stage(child_2, level)) nodes.push_back(child_2);
                        tree_growed = true;
                        LOG_NODE(*child_2);
                    }
                }
            } else {
                if (node->op.end+1 < state.size() && state[node->op.end+1] == node->op.cur) {
                    auto[char_1, char_2] = convert_double_letter(node->op.cur);
                    NodePtr child_1 = make_node(Node::Type::Dependency, Node::Operation{
                            .start = node->op.start,
                            .end = node->op.end+1,
                            .prev = node->op.cur,
                            .cur = char_1,
                            }, node->level);
                    LOG_NODE(*child_1);
                    NodePtr child_2 = make_node(Node::Type::Dependency, Node::Operation{
                            .start = node->op.start,
                            .end = node->op.end+1,
                            .prev = node->op.cur,
                            .cur = char_2,
                            }, node->level);
                    node->children.push_back(child_1);
                    node->children.push_back(child_2);
                    if (!dependency_stage(child_1, level)) nodes.push_back(child_1);
                    if (!dependency_stage(child_2, level)) nodes.push_back(child_2);
                    tree_growed = true;
                    LOG_NODE(*child_2);
                }
                if (node->op.start >= 1 && state[node->op.start-1] == node->op.cur) {
                    auto[char_1, char_2] = convert_double_letter(node->op.cur);
                    NodePtr child_1 = make_node(Node::Type::Dependency, Node::Operation{
                            .start = node->op.start-1,
                            .end = node->op.end,
                            .prev = node->op.cur,
                            .cur = char_1,
                            }, node->level);
                    LOG_NODE(*child_1);
                    NodePtr child_2 = make_node(Node::Type::Dependency, Node::Operation{
                            .start = node->op.start-1,
                            .end = node->op.end,
                            .prev = node->op.cur,
                            .cur = char_2,
                            }, node->level);
                    node->children.push_back(child_1);
                    node->children.push_back(child_2);
                    if (!dependency_stage(child_1, level)) nodes.push_back(child_1);
                    if (!dependency_stage(child_2, level)) nodes.push_back(child_2);
                    tree_growed = true;
                    LOG_NODE(*child_2);
                }
            }
        } else {
            for (NodePtr& child : node->children) {
                tree_growed |= dependency_stage(child, level);
            }
        }
        tree_size_growed |= tree_growed;
        return tree_growed;
    }

    bool tree_size_growed;
    NodePtr root;
    std::string state;
    std::vector<NodePtr> nodes;
};


int main() {
    const char* state = "aaccadd";
    printf("%s\n", state);
    DBSearcher searcher(state);
    searcher.search();
    return 0;
}
