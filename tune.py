import subprocess

import run

def main():
    
    max_lookahead = [5, 6, 7]
    search_threshold = [0.006, 0.005, 0.004, 0.003, 0.002]
    maybe_dead_threshold = [12, 14, 16, 18]

    run.init()
    #for a in max_lookahead:
    #    for b in search_threshold:
    #        for c in maybe_dead_threshold:
    for a,b,c in [
            (5,0.010,12),
            (4,0.010,10),
            (5,0.010,16),
            #(6,0.006,16),
            #(6,0.006,18),
            #(6,0.004,18),
            #(6,0.002,18),
            #(7,0.006,12),
            #(7,0.006,14),
            #(7,0.006,16),
            #(7,0.006,18),
            #(7,0.006,20),
            (7,0.006,22),
            #(7,0.005,16),
            #(7,0.005,18),
            #(7,0.003,12),
            #(7,0.003,14),
            ]:
                print a,b,c
                run.extra_args = ['-p', '%d,%f,%d' % (a,b,c)]
                run.run_jobs(300)
                print

if __name__ == '__main__':
    main()
