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

free_server = multiprocessing.Queue()
extra_args = []

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
    r = urllib2.urlopen(url, data=data)
    result = json.loads(r.read())
    #print result
    assert result['done'] == 'ok'
    return result['output'].splitlines()

def func((exe, exe_md5, idx)):
    cmd = ['./2048', '-s', str(idx+2000)] + extra_args

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
    print idx, result

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

def query_servers():
    result = []
    for s in server_lists:
        r = urllib2.urlopen('http://%s/config' % s)
        x = r.read()
        print s, int(x)
        result += [s] * int(x)
    return result

def init():
    for s in query_servers():
        free_server.put(s)
    ncpu = free_server.qsize()
    print 'total cpu', ncpu

def run_jobs(njob):
    ncpu = free_server.qsize()
    exe = file('2048', 'rb').read()
    exe_md5 = md5(exe)
    jobs = [(exe,exe_md5,i) for i in range(njob)]
    if njob == 1 or ncpu == 1:
        result = map(func, jobs)
    else:
        pool = multiprocessing.dummy.Pool(processes=ncpu)
        result = pool.map(func, jobs)

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
    #print [round(t*1000,1) for t in ts]
    print 'rank', [round(p*100, 1) for p in count_ranks(ranks)]
    #if len(scores) > 5:
    #    scores = scores[2:-2] # remove low 2 and high 2
    print 'median %d, avg %.1f, %.2fms/move, weighted %.2fms/move' % (
            scores[len(scores)/2],
            sum(scores)/len(scores),
            1000.*sum(ts) / len(ts),
            1000.*sum(totaltime) / sum(moves),
            )

def main():
    init()
    ncpu = free_server.qsize()
    njob = ncpu
    if sys.argv[1:]:
        njob = int(sys.argv[1])
    run_jobs(njob)


if __name__ == '__main__':
    main()
