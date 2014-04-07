import numpy

"""

A -> B -> C
isInt -> hasAdd -> hasIncr

P(A,B,C) = P(A)P(B|A)P(C|B)

"""
def combine(nodes,ndim=2):
    
    if len(nodes) == 1:
        return 


class Network:
    def __init__(self):
        self.nodes = {}
    def p(self, X):
        return
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
        self.children = {}
    def addChild(self, c):
        self.net.nodes[c.name] = c
        self.children[c.name] = c
        self.p = numpy.random.random([2]*len(self.children))*0+0.4
        
# 1 observed true, 0 observed false, -1 unobserved

data = numpy.array([[1,0,1],
                    [1,1,1],
                    [1,0,1],
                    [1,1,0],
                    [0,0,0],
                    [0,1,0]])

A = Node("A")
B = Node("B")
C = Node("C")
A.addChild(B)
B.addChild(C)
