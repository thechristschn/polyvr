#ifndef GRAPH_H_INCLUDED
#define GRAPH_H_INCLUDED

#include <vector>
#include <OpenSG/OSGVector.h>
#include "core/math/VRMathFwd.h"
#include "core/math/boundingbox.h"

using namespace std;
OSG_BEGIN_NAMESPACE;

class graph_base {
    public:
        enum CONNECTION {
            SIMPLE,
            HIERARCHY,
            DEPENDENCY,
            SIBLING
        };

        struct node {
            boundingbox box;
        };

        struct edge {
            int from;
            int to;
            CONNECTION connection;

            edge(int i, int j, CONNECTION c);
        };

        struct emptyNode {
            void update(node& n, bool changed) {}
        };

    protected:
        vector< vector<edge> > edges;
        vector< node > nodes;

    public:
        graph_base();
        ~graph_base();

        void connect(int i, int j, CONNECTION c = SIMPLE);
        node& getNode(int i);
        vector< node >& getNodes();
        vector< vector<edge> >& getEdges();
        int getNEdges();
        int size();

        //vector<node>::iterator begin();
        //vector<node>::iterator end();

        void setPosition(int i, Vec3f v);
        virtual void update(int i, bool changed);
        virtual void clear();
};

template<class T>
class graph : public graph_base {
    private:
        vector<T> elements;

        void update(int i, bool changed);

    public:
        graph();
        ~graph();

        static shared_ptr< graph<T> > create();

        int addNode();
        int addNode(T t);

        vector<T>& getElements();
        T& getElement(int i);
        void clear();
};

OSG_END_NAMESPACE;

#endif // GRAPH_H_INCLUDED
