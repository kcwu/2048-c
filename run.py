import multiprocessing
import multiprocessing.dummy
import subprocess
import re
import time
import sys
import os
import json
import hashlib
import urllib2
import base64
import random
import datetime

free_server = multiprocessing.Queue()
extra_args = []
#extra_args = ['-p', 'reverse=1.5,2.0,1.0650918863779966,1.0,1.129', '-p', 'equal=0.0', '-p', 'inc=-0.029889330949802943,-0.064,4.0,0.0', '-p', 'smooth=2.0,1.0,1.0,1.04437905327448', '-p', 'blank=0.0,1.0,0.0']

def md5(s):
    m = hashlib.md5()
    m.update(s)
    return m.hexdigest()

def run(server, exe, arg):
    p = subprocess.Popen(cmd, stdout=subprocess.PIPE)
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
    return lines

def run(server, exe, arg):
    url = 'http://%s/run' % server
    data = json.dumps([base64.b64encode(exe), arg])
    try:
        r = urllib2.urlopen(url, data=data)
    except urllib2.URLError:
        print 'error', url
        raise
    result = json.loads(r.read())
    print result
    assert result['done'] == 'ok'
    return result['output'].splitlines()

def func((exe, exe_md5, idx)):
    seed_base = int(md5(str(extra_args)+exe_md5)[:6], 16)
    cmd = ['./2048', '-s', str(idx+seed_base)] + extra_args

    #exe = file('2048', 'rb').read()
    #exe_md5 = md5(exe)
    cmd_md5 = md5(json.dumps(cmd))
    cache_fn = 'cache/%s/%s' % (exe_md5, cmd_md5)
    if os.path.exists(cache_fn):
        return json.load(file(cache_fn))

    server = free_server.get()
    arg = cmd[1:]
    lines = run(server, exe, arg)
    m = re.match(
            r'time=(\d+\.\d+), final score=(\d+), moves=(\d+), max tile=(\d+)',
            lines[-1])
    assert m

    t0 = 0
    t1 = float(m.group(1))
    score = int(m.group(2))
    moves = int(m.group(3))
    maxtile = int(m.group(4))
    result = (t1-t0), moves, (t1-t0)/moves, score, maxtile
    #print idx, result

    try:
        os.mkdir('cache/%s' % exe_md5)
    except OSError:
        pass
    with file(cache_fn, 'w') as f:
        json.dump(result, f)

    free_server.put(server)

    return result


def count_ranks(ranks):
    def larger(x):
        return 1.0*len([r for r in ranks if r >= x]) / len(ranks)
    return larger(11), larger(12), larger(13), larger(14), larger(15)

server_lists = [
        'localhost:8765',
        ] + re.findall(r'[^,]+', os.environ.get('SERVER_2048', ''))

def query_servers(verbose=True):
    result = []
    summary = []
    for s in server_lists:
        r = urllib2.urlopen('http://%s/config' % s)
        x = r.read()
        if 5 <= datetime.date.today().weekday() <= 6:
            if verbose:
                print '\tweekend force 30'
            x = 30
        if verbose:
            print s, int(x)
        summary.append('%s*%d' % (s, int(x)))
        result += [s] * int(x)
    #print ' '.join(summary)
    random.shuffle(result)
    return result

def init(verbose=True):
    while not free_server.empty():
        free_server.get()
    assert free_server.qsize() == 0

    for s in query_servers(verbose):
        free_server.put(s)
    ncpu = free_server.qsize()
    print 'total cpu', ncpu

def run_jobs(njob):
    init(verbose=False)

    ncpu = free_server.qsize()
    exe = file('2048', 'rb').read()
    exe_md5 = md5(exe)
    jobs = [(exe,exe_md5,i) for i in range(njob)]
    if njob == 1 or ncpu == 1:
        result = map(func, jobs)
    else:
        pool = multiprocessing.dummy.Pool(processes=ncpu)
        result = pool.map(func, jobs)
        pool.close()
        pool.join()

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

    rank_summary = [round(p*100, 1) for p in count_ranks(ranks)]
    #print scores
    #print [round(t*1000,1) for t in ts]
    print 'rank', rank_summary
    #if len(scores) > 5:
    #    scores = scores[2:-2] # remove low 2 and high 2
    print 'median %d, avg %.1f, %.2fms/move, weighted %.2fms/move' % (
            scores[len(scores)/2],
            sum(scores)/len(scores),
            1000.*sum(ts) / len(ts),
            1000.*sum(totaltime) / sum(moves),
            )
    return rank_summary

def main():
    init()
    ncpu = free_server.qsize()
    njob = ncpu
    if sys.argv[1:]:
        njob = int(sys.argv[1])
    run_jobs(njob)


if __name__ == '__main__':
    main()
