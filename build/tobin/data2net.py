#!/usr/bin/env python

import sys, subprocess, pathlib

def build_cmd(args):
    nof_vars = len(open(args.vdfile).readlines())
    bindir = pathlib.Path(__file__).parent
    ext = '.exe' if sys.platform == 'win32' else ''
    rdir = pathlib.Path(args.resdir)

    options = ' '.join(f"{name} {value}" for name,value in vars(args).items() 
                       if not name in ('vdfile', 'datfile', 'score', 'resdir') and not value is None)
    cmd = f"""
    {bindir}/get_local_scores{ext} {args.vdfile} {args.datfile} {args.score} {rdir}/res {options}
    {bindir}/split_local_scores{ext}   {nof_vars} {rdir}
    {bindir}/reverse_local_scores{ext} {nof_vars} {rdir}
    {bindir}/get_best_parents{ext}     {nof_vars} {rdir}
    {bindir}/get_best_sinks{ext}       {nof_vars} {rdir} {rdir}/sinks
    {bindir}/get_best_order{ext}       {nof_vars} {rdir}/sinks {rdir}/ord
    {bindir}/get_best_net{ext}         {nof_vars} {rdir} {rdir}/ord {rdir}/net
    {bindir}/score_net{ext}            {rdir}/net {rdir}
    """
    print(cmd)

    return cmd

def main(args):
    try:  
        rdir = pathlib.Path(args.resdir)
        rdir.mkdir(parents=True, exist_ok=True)
    except:
        raise
    cmd = build_cmd(args)
    subprocess.call(cmd, shell=True)

def add_arguments(parser):
    parser.add_argument('vdfile')
    parser.add_argument('datfile')
    parser.add_argument('score')
    parser.add_argument('resdir')
    parser.add_argument('--nof-tasks', type=int)
    parser.add_argument('--task-index', type=int)
    parser.add_argument('-c', '--constraints')
    # should add more

if __name__ == '__main__':
    from argparse import ArgumentParser

    parser = ArgumentParser()
    add_arguments(parser)
    args = parser.parse_args()
    main(args)
