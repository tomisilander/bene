#!/usr/bin/python

import array
import operator
import sys
import os
from functools import reduce
from itertools import islice


def product(xs):
    return reduce(operator.mul, xs, 1)

def nof_subsets(n,k):
    return product(range(n-k+1,n+1)) // product(range(1,k+1))

def nof_scores(n,k):
    return sum([nof_subsets(n,k_)*k_ for k_ in range(1,k+1)])

def gen_scores(scorefile, n, k):
    statinfo = os.stat(scorefile)
    to_read  = nof_scores(n,k)
    t = 'd' if statinfo.st_size/to_read == 8 else 'f'
    blocksize = 1024
    f = open(scorefile,'rb')
    while to_read > 0:
        a = array.array(t)
        readsize = min(to_read, blocksize)
        a.fromfile(f,readsize)
        for s in a:
            yield s
        to_read -= readsize
    f.close()


def write_file(fn,l,len_l):
    f = open(fn,'a')
    for (parents, score) in islice(l,len_l):
        nof_pars     = len(parents)
        parents_str  = ' '.join(map(str, parents))
        f.write("%f %d %s\n" % (score, nof_pars, parents_str))
    f.close()
    
def printer(scorefile, n, k, resdir):
    """Keep n lists of (parentcfg, score)-pairs
    when list is full, dump it to the file"""

    listlen = 1024*1024
    ixs = [0]*n
    lists = [[()]*listlen for i in range(n)]

    try:
        os.makedirs(resdir)
    except:
        pass
    
    filenames = [os.path.join(resdir, "%04d"%i) for i in range(n)]
    ns = nof_scores(n,k)//n
    for i,fn in enumerate(filenames):
        f=open(fn,'w');
        f.write("%d %d\n"%(i,ns))
        f.close()
        
    for s in  gen_scores(scorefile,n,k):
        (i,parents) = (yield)
        lists[i][ixs[i]] = (parents,s)
        ixs[i] += 1
        if ixs[i] == listlen:
            write_file(filenames[i], lists[i], ixs[i])
            ixs[i]=0
    for i in range(n):
        write_file(filenames[i], lists[i], ixs[i])
        
    x = (yield) #why is this needed?

def get_conds(vs):
    for v in vs:
        pr.send((v,[v2 for v2 in vs if v2!=v]))
        
def walk(vs, call_count):
    get_conds(vs)
    for i in range(call_count):
        vs2 = vs[:]
        del vs2[i]
        walk(vs2, i)

def comb(n,c, m):
    """ n = maximal element that can be added
        c = elements already in place
        m = the smallest thing skipped - used to control recursion down
"""

    if c == k:
        #cp.send((sel, 0 if sel[0]>0 else m))
        walk(sel, 0 if sel[0]>0 else m)
        return

    # add largest element and recurse
    sel[k-1-c] = n # add to end
    comb(n-1, c+1,m)

    # do not add n but recurse
    if n+1>k-c: # we can still decrease n
        comb(n-1, c,n)

if len(sys.argv) != 5:
    print("Usage: %s nof_vars max_nof_parents scorefile result_dir"%sys.argv[0])
    sys.exit(1)


n,max_parents = map(int, sys.argv[1:3])
k = max_parents+1
scorefile,resdir = sys.argv[3:]
sel = [-1]*k
pr = printer(scorefile, n, k, resdir)
next(pr)
comb(n-1,0,n)
