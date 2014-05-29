import multiprocessing
import subprocess
import re
import time
import sys
import os

def func(idx):
    t0 = time.time()
    p = subprocess.Popen(['./2048', '-s', str(idx+2000)], stdout=subprocess.PIPE)
    lines = []

    if 0:
        # how to make it buffer less?
        for line in p.stdout:
            if idx == 0:
                sys.stdout.write(line)
            lines.append(line)
    else:
        stdout = p.stdout.fileno()
        lastlines = ''
        while True:
            line = os.read(stdout, 1024)
            if not line:
                break
            if idx == 0:
                sys.stdout.write(line)
            if line.count('\n') > 1:
                lastlines = line
            else:
                lastlines += line
            lines = lastlines.splitlines()
            #lines.append(line)
    p.wait()
    t1 = time.time()
    m = re.match(
            r'final score=(\d+), moves=(\d+), max tile=(\d+)',
            lines[-1])
    assert m
    
    score = int(m.group(1))
    moves = int(m.group(2))
    maxtile = int(m.group(3))
    print (t1-t0), moves, (t1-t0)/moves, score, maxtile

    return t1-t0, moves, (t1-t0)/moves, score, maxtile

def count_ranks(ranks):
    def larger(x):
        return 1.0*len([r for r in ranks if r >= x]) / len(ranks)
    return larger(11), larger(12), larger(13), larger(14), larger(15)

def main():
    ncpu = 30
    njob = ncpu
    if sys.argv[1:]:
        njob = int(sys.argv[1])
    if njob == 1 or ncpu == 1:
        result = map(func, range(njob))
    else:
        pool = multiprocessing.Pool(processes=ncpu)
        result = pool.map(func, range(njob))

    totaltime = []
    moves = []
    scores = []
    ts = []
    ranks = []
    for r in result:
        totaltime.append(r[0])
        moves.append(r[1])
        ts.append(r[2])
        scores.append(r[3])
        ranks.append(r[4])

    ts.sort()
    scores.sort()
    ranks.sort()

    print scores
    print [round(t*1000,1) for t in ts]
    print 'rank', [round(p*100, 1) for p in count_ranks(ranks)]
    if len(scores) > 5:
        scores = scores[2:-2] # remove low 2 and high 2
    print 'median %d, avg %.1f, %.2fms/move, weighted %.2fms/move' % (
            scores[len(scores)/2], 
            sum(scores)/len(scores),
            1000.*sum(ts) / len(ts),
            1000.*sum(totaltime) / sum(moves),
            )


main()
