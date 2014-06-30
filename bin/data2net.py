#!/usr/bin/python

import sys, os, subprocess

def main(vdfile, datfile, score, resdir,
         nof_tasks=1, task_index=0, cstrfile=None):
    nof_vars = len(open(vdfile).readlines())
    bindir=os.path.dirname(__file__)
    
    kws = {'binpath/'   : os.path.join(bindir,''),
           'nof_vars'   : str(nof_vars),
           'rdir'       : resdir,
           'rdir/'      : os.path.join(resdir,''),
           'vdfile'     : vdfile,
           'datfile'    : datfile,
           'score'      : score,
           'nof_tasks'  : '--nof-tasks %d' % nof_tasks,
           'task_index' : '--task-index %d' % task_index,
           'cstr'       : '-c %s' % cstrfile if cstrfile != None else '',
           'ext'        : '.exe' if sys.platform == 'win32' else ''
}

    scorecmd = """
{binpath/}get_local_scores{ext} {vdfile} {datfile} {score} {rdir/}res {nof_tasks} {task_index} {cstr}
{binpath/}split_local_scores{ext}   {nof_vars} {rdir}
{binpath/}reverse_local_scores{ext} {nof_vars} {rdir}
{binpath/}get_best_parents{ext}     {nof_vars} {rdir}
{binpath/}get_best_sinks{ext}       {nof_vars} {rdir} {rdir/}sinks
{binpath/}get_best_order{ext}       {nof_vars} {rdir}/sinks {rdir/}ord
{binpath/}get_best_net{ext}         {nof_vars} {rdir} {rdir/}ord {rdir/}net
{binpath/}score_net{ext}            {rdir/}net {rdir}
""".format(**kws) 
        
    if os.path.exists(resdir):
        if not os.path.isdir(resdir):
            sys.exit('%s is not a directory' % resir)
    else:
        os.makedirs(resdir)
    subprocess.call(scorecmd, shell=True)

from coliche import che
che(main,
    """vdfile;datfile;score;resdir; 
      --nof-tasks nof_tasks(int): default: 1
      --task-index task_index: default: 0
      -c --constraints cstrfile
""")
