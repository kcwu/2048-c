import subprocess
import random

import run

iteration = 0
random.seed(0)
population = 100
crossover_rate = 0.5
mutation_rate = 0.05
num_tournament = 10

seeds = [
        [1.5, 2.0, 1.0650918863779966, 1.0, 0.9587734262814023, 0.0, -0.029889330949802943, 0.0, 4.0, 0.0, 2.0, 1.0, 1.0, 1.04437905327448, 0.0, 1.0, 0.0],
        [1.5, 2.0, 1.0650918863779966, 1.0, 0.9622089740912855, 0.0, -0.029889330949802943, 0.0, 4.0, 0.0, 2.0, 1.0, 1.0, 1.04437905327448, 0.0, 1.0, 0.0],
        [1.5, 2.0, 1.0650918863779966, 1.0, 0.9519023306616361, 0.0, -0.029889330949802943, 0.0, 4.0, 0.0, 2.0, 1.0, 1.0, 1.04437905327448, 0.0, 1.0, 0.0],
        [1.5, 2.0, 1.0650918863779966, 1.0, 0.9793867131407011, 0.0, -0.029889330949802943, 0.0, 4.0, 0.0, 2.0, 1.0, 1.0, 1.04437905327448, 0.0, 1.0, 0.0],
        ]

class Para:
    def __init__(self):
        self.p = [
                # reverse
                1.5,
                2.0,
                1.0,
                1.0,
                1.0,
                # equal
                0.0,
                # inc
                0.0,
                0.0,
                4.0,
                0.0,
                # smooth
                2.0,
                1.0,
                1.0,
                1.0,
                # blank
                0.0,
                1.0,
                0.0,
        ]
    def clone(self):
        p = Para()
        p.p = list(self.p)
        return p

    def mutate(self):
        i = random.randint(0, len(self.p)-1)
        v = 0.1
        if i == 1: # para_reverse
            v = 0.01
        if i == 10: # para_smooth
            v = 0.01

        self.p[i] += random.normalvariate(0, v)
        self.p[i] = round(self.p[i], 3)

    def crossover(self, p2):
        assert p2
        p = Para()
        for i in range(len(self.p)):
            p.p[i] = round((self.p[i] + p2.p[i]) / 2.0, 3)
        return p

    def to_args(self):
        if iteration > 41:
            return [
                    '-p', 'reverse=%r,%r,%r,%r,%r' % tuple(self.p[0:5]),
                    '-p', 'equal=%r' % self.p[5],
                    '-p', 'inc=%r,%r,%r,%r' % tuple(self.p[6:10]),
                    '-p', 'smooth=%r,%r,%r,%r' % tuple(self.p[10:14]),
                    '-p', 'blank=%r,%r,%r' % tuple(self.p[14:17]),
                    ]
        return [
                '-p', 'reverse=%f,%f,%f,%f,%f' % tuple(self.p[0:5]),
                '-p', 'equal=%f' % self.p[5],
                '-p', 'inc=%f,%f,%f,%f' % tuple(self.p[6:10]),
                '-p', 'smooth=%f,%f,%f,%f' % tuple(self.p[10:14]),
                '-p', 'blank=%f,%f,%f' % tuple(self.p[14:17]),
                ]

    def __eq__(self, b):
        for i in range(len(self.p)):
            if abs(b.p[i] - self.p[i]) > 0.001:
                return False
        return True

class Bot:
    def __init__(self, p):
        self.p = p
        self.score = [0]

    def clone(self):
        p = Bot(self.p.clone())
        return p

    def run(self):
        run.extra_args = self.p.to_args()
        print ' '.join(run.extra_args)
        self.score = run.run_jobs(200)
        assert sum(self.score) > 0

def uniq(bots):
    result = []
    for b in bots:
        for r in result:
            if r.p == b.p:
                break
        else:
            result.append(b)
    return result

def main():
    global iteration
    run.init()

    bots = []
    b = Bot(Para())
    b.run()
    bots.append(b)
    for s in seeds:
        b = Bot(Para())
        b.p.p = list(s)
        b.run()
        bots.append(b)

    iteration = 0
    while True:
        iteration += 1
        print 'iteration', iteration
        
        crossover_count = 5
        more_bots = []
        for i in range(crossover_count):
            b1 = Bot(None)
            b2 = Bot(None)
            num_tournament = 3
            for i in range(num_tournament):
                b = random.choice(bots)
                if sum(b.score) > sum(b1.score):
                    b1 = b
                b = random.choice(bots)
                if sum(b.score) > sum(b2.score):
                    b2 = b

            winner = Bot(b1.p.crossover(b2.p))
            more_bots.append(winner)

        bots = uniq(bots + more_bots)
        for b in bots:
            if b.score == [0]:
                b.run()

        bots.sort(key=lambda p: sum(p.score), reverse=True)
        bots = bots[:population]

        # mutation
        for b in bots:
            if random.random() < mutation_rate:
                b = b.clone()
                b.p.mutate()
                bots.append(b)

        bots = uniq(bots)
        for b in bots:
            if b.score == [0]:
                b.run()

        print '-'*30
        print [sum(b.score) for b in bots]
        print bots[0].score, bots[0].p.to_args()

        for b in bots[:10]:
            print b.p.p
        print

if __name__ == '__main__':
    main()
