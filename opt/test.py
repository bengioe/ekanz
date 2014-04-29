import time
import numpy

"""

A -> B -> C
       -> D

P(A) = sum_BCD P(A)P(B|A)P(C|B)P(D|B)
     = P(A) sum_B P(B|A) sum_C P(C|B) sum_D P(D|B)

"""
def combine(nodes,ndim=2):
    
    if len(nodes) == 1:
        return 


class Network:
    def __init__(self):
        self.nodes = {}
    def __getitem__(self, i):
        return self.nodes[i]

    def p(self, x, givens):
        X = self.nodes[x]
        givens = givens.split()
        #print x, X, X.parents, X.children
        #sfactors = ["P("+i+"|"+",".join(self.nodes[i].parents)+")" for i in self.nodes]
        #print sfactors, givens
        factors = [([i]+self.nodes[i].porder,
                    self.nodes[i].p)
                    for i in self.nodes]
        #print factors
        p = 1
        for i,n in self.nodes.iteritems():
            if i == x: continue
            #print "P("+i+"|"+",".join(self.nodes[i].parents)+")"
            new_factor = []
            for fi,f in enumerate(factors):
                if i in f[0]:
                    factors[fi] = 0
                    pos = f[0].index(i)
                    #print "reducing:",f,"from",i,pos,"to",
                    if pos == 0:
                        if i in givens:
                            new_factor.append((f[0][1:],f[1]))
                        else:
                            new_factor.append((f[0][1:],f[1]+1-f[1]))
                    else:
                        if i in givens:
                            new_factor.append(([j for j in f[0] if j != i],
                                               f[1][[slice(f[1].shape[j]) if j != pos else 0
                                                     for j in range(1,f[1].ndim+1)]]))
                        else:
                            new_factor.append(([j for j in f[0] if j != i],
                                               f[1].sum(axis=pos-1)))
                    #print new_factor[-1]
            while 0 in factors:
                factors.pop(factors.index(0))
            factors += new_factor
        #print "\n".join(map(str,factors))
        for i,f in enumerate(factors):
            if type(f[1]) != float and type(f[1]) != numpy.float64:
                #print f,type(f[1])
                #assert f[0][0] == x and len(f[0]) == 1
                factors[i] = [[],f[1].mean()]
        #print factors
        return reduce(lambda a,b:a*b,[i[1] for i in factors])

    def plot(self):
        import numpy as np
        import networkx as nx
        import matplotlib.pyplot as plt

        edges = []
        pos = {}
        def depth(n):
            if len(n.parents):
                return max([depth(n.parents[i]) for i in n.parents]) + 1
            return 1
        dcounts = [0 for i in range(10)]
        for i in self.nodes:
            n = self.nodes[i]
            d = depth(n)
            pos[i] = (0*np.random.random()+dcounts[d]-d*3,
                      0*np.random.random() - 10*d+len(n.children)*2 - len(n.parents)*2)
            dcounts[d] += 10

            for j in self.nodes[i].children:
                edges.append((i,j))


        g = nx.DiGraph()
        g.add_edges_from(edges)
        #pos =  nx.spring_layout(g,scale=1000)
        nx.draw(g,pos,node_size = 1000, node_color='w',cmap={'int':0.9})
        plt.show()

class Node:
    """
    node.p is stored as follows:
    P(N  | C1, ..., Ck) = node.p[0, ..., 0]
    P(!N | C1, ..., Ck) = 1 - node.p[0, ...,0]
    P(N  |!C1, ..., Ck) = node.p[1, ...,0]
    """
    def __init__(self, name="<Node>", network=None):
        self.net = network
        self.name = name
        self.p = numpy.random.random()
        self.porder = []
        self.children = {}
        self.parents = {}
        self.net.nodes[name] = self
    def addChild(self, c):
        self.net.nodes[c.name] = c
        self.children[c.name] = c
        c.parents[self.name] = self
        c.porder.append(self.name)
        c.p = numpy.random.random([2]*len(c.parents))#*0+0.01*numpy.array(range(1,1+2**len(c.parents))).reshape([2]*len(c.parents))
    def __str__(self):
        return "<"+self.name+">"
    def __repr__(self):
        return str(self)

# 1 observed true, 0 observed false, -1 unobserved

data = numpy.array([[1,0,1],
                    [1,1,1],
                    [1,0,1],
                    [1,1,0],
                    [0,0,0],
                    [0,1,0]])

if 1:
    net = Network()
    keys = "add sub mul div mod bitw attr indx call eq ineq len range for iter indxee callee if while ret res loc 1l int float str list tup dict obj".split()
    for i in keys:
        Node(i, net)

    link = lambda p,c: [net[p].addChild(net[i]) for i in c.split()]

    link("int", "mod bitw ineq eq indxee 1l range loc")
    link("bitw", "add sub mul div")
    link("mod", "add sub mul div")
    link("float", "add sub mul div ineq loc")
    link("str", "add attr indx eq len call ret")
    link("list", "indx len for iter attr")
    link("tup", "indx iter ret loc")
    link("dict", "indx for iter ret")
    link("obj", "attr tup dict list")

    #net.p("int","add sub indxee")

    net.plot()
    import time
    t0 = time.time()
    p = net.p("int","add sub mul ret")
    t0 = time.time()-t0
    print "took", t0*1000,"ms"
    print p

if 0:
    net = Network()
    Node("A", net)
    Node("B", net)
    Node("C", net)
    net["A"].addChild(net["B"])
    net["C"].addChild(net["B"])
    import time
    t0 = time.time()
    p = net.p("A","C")
    print time.time()-t0
    print p
