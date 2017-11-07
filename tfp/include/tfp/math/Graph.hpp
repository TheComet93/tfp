#pragma once

#include "tfp/util/Reference.hpp"
#include <unordered_map>
#include <memory>

namespace tfp {

class Node;
class Connection;
class VariableTable;

class Graph : public RefCounted
{
public:
    struct PathSegment
    {
        Node* in_;
        Node* out_;
        Connection* connection_;
    };

    typedef std::vector<PathSegment> Path;
    typedef std::vector<Path> PathList;
    typedef std::vector<Node*> NodeList;

    Graph();

    Node* createNode(const char* name);
    Node* findNode(const char* name);

    void setForwardPath(Node* in, Node* out);
    VariableTable* variables();

    Expression* mason() const;

    bool evaluatePhysicalUnitConsistencies() const;

    void findForwardPathsAndLoops(PathList* paths, PathList* loops) const;
    void findForwardPathsAndLoopsRecursive(PathList* paths, PathList* loops,
                                           Node* current, NodeList list) const;
    void nodeListToPath(Path* path, const NodeList& nodes) const;
    Expression* calculateConnectionGain(const Path& path) const;
    Expression* calculateDeterminant(const PathList& loops) const;
    Expression* calculateCofactorsAndPathGains(const PathList& loops, const PathList& paths) const;
    bool pathsAreTouching(const Path& a, const Path& b) const;

    void dump(const char* fileName) const;
    void dump(FILE* fileName) const;

private:
    Node* input_;
    Node* output_;
    std::unordered_map< std::string, std::unique_ptr<Node> > nodes_;
    Reference<VariableTable> variables_;
};

} // namespace tfp