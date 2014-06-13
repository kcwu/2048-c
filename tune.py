import subprocess
import random

import run

iteration = 0
random.seed(0)
population = 20
crossover_rate = 0.5
mutation_rate = 0.2
num_tournament = 5

seeds = [

        [1.525, 2.247, 1.0, 1.0, 1.0, 0.0, 0.0, 0.0, 3.486, 0.0, 1.971, 1.0, 1.0, 1.0, 0.0, 1.0, 0.0] ,
        #[1.524, 2.203, 1.0, 1.0, 1.0, 0.0, 0.0, 0.0, 3.38, 0.0, 1.973, 1.0, 1.0, 1.0, 0.0, 1.0, 0.0] ,
        #[2.309, 2.203, 1.0, 1.0, 1.0, 0.0, 0.0, 0.0, 3.38, 0.0, 1.973, 1.0, 1.0, 1.0, 0.0, 1.0, 0.0] ,
        #[1.523, 2.277, 1.0, 1.0, 1.0, 0.0, 0.0, 0.0, 3.36, 0.0, 1.974, 1.0, 1.0, 1.0, 0.0, 1.0, 0.0] ,
        #[1.523, 2.203, 1.0, 1.0, 1.0, 0.0, 0.0, 0.0, 3.36, 0.0, 1.974, 1.0, 1.0, 1.0, 0.0, 1.0, 0.0] ,
        #[1.373, 2.257, 1.0, 1.0, 1.0, 0.0, 0.0, 0.0, 3.385, 0.0, 2.006, 1.0, 1.0, 1.0, 0.0, 1.0, 0.0] ,
        #[1.8, 2.203, 1.0, 1.0, 1.0, 0.0, 0.0, 0.0, 3.36, 0.0, 1.974, 1.0, 1.0, 1.0, 0.0, 1.0, 0.0] ,
        #[1.526, 2.165, 1.0, 1.0, 1.0, 0.0, 0.0, 0.0, 3.414, 0.0, 1.97, 1.0, 1.0, 1.0, 0.0, 1.0, 0.0] ,
        #[1.67, 2.277, 1.0, 1.0, 1.0, 0.0, 0.0, 0.0, 3.36, 0.0, 1.974, 1.0, 1.0, 1.0, 0.0, 1.0, 0.0] ,
        #[1.528, 2.093, 1.0, 1.0, 1.0, 0.0, 0.0, 0.0, 3.468, 0.0, 1.965, 1.0, 1.0, 1.0, 0.0, 1.0, 0.0] ,
        #[1.523, 2.305, 1.0, 1.0, 1.0, 0.0, 0.0, 0.0, 3.109, 0.0, 1.974, 1.0, 1.0, 1.0, 0.0, 1.0, 0.0] ,
        #[1.528, 2.093, 1.0, 1.0, 1.0, 0.0, 0.0, 0.0, 3.468, 0.0, 1.937, 1.0, 1.0, 1.0, 0.0, 1.0, 0.0] ,
        #[1.523, 2.213, 1.0, 1.0, 1.0, 0.0, 0.0, 0.0, 3.46, 0.0, 1.974, 1.0, 1.0, 1.0, 0.0, 1.0, 0.0] ,
        #[1.449, 2.23, 1.0, 1.0, 1.0, 0.0, 0.0, 0.0, 3.382, 0.0, 2.008, 1.0, 1.0, 1.0, 0.0, 1.0, 0.0] ,
        #[1.525, 2.238, 1.0, 1.0, 1.0, 0.0, 0.0, 0.0, 3.486, 0.0, 1.971, 1.0, 1.0, 1.0, 0.0, 1.0, 0.0] ,
        #[1.917, 2.225, 1.0, 1.0, 1.0, 0.0, 0.0, 0.0, 3.433, 0.0, 1.972, 1.0, 1.0, 1.0, 0.0, 1.0, 0.0] ,
        #[1.523, 2.237, 1.0, 1.0, 1.0, 0.0, 0.0, 0.0, 3.36, 0.0, 1.974, 1.0, 1.0, 1.0, 0.0, 1.0, 0.0] ,
        #[1.525, 2.203, 1.0, 1.0, 1.0, 0.0, 0.0, 0.0, 3.401, 0.0, 1.971, 1.0, 1.0, 1.0, 0.0, 1.0, 0.0] ,
        #[1.525, 2.203, 1.0, 1.0, 1.0, 0.0, 0.0, 0.0, 3.486, 0.0, 1.971, 1.0, 1.0, 1.0, 0.0, 1.0, 0.0] ,
        #[1.664, 2.148, 1.0, 1.0, 1.0, 0.0, 0.0, 0.0, 3.414, 0.0, 1.956, 1.0, 1.0, 1.0, 0.0, 1.0, 0.0] ,


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
        i = random.choice([0, 1, 8, 10])
        i = 5
        v = 0.5
        if i in (1, 10):
            v = 0.1
        if i == 5:
            v = 5;

        self.p[i] += random.normalvariate(0, v)
        self.p[i] = round(self.p[i], 3)

    def crossover(self, p2):
        assert p2
        p = Para()
        for i in range(len(self.p)):
            p.p[i] = round((self.p[i] + p2.p[i]) / 2.0, 3)
        return p

    def to_args(self):
        return [
                '-p', 'reverse=%r,%r,%r,%r,%r' % tuple(self.p[0:5]),
                '-p', 'equal=%r' % self.p[5],
                '-p', 'inc=%r,%r,%r,%r' % tuple(self.p[6:10]),
                '-p', 'smooth=%r,%r,%r,%r' % tuple(self.p[10:14]),
                '-p', 'blank=%r,%r,%r' % tuple(self.p[14:17]),
                ]

    def __eq__(self, b):
        for i in range(len(self.p)):
            if i in (1, 10):
                if abs(b.p[i] - self.p[i]) > 0.001:
                    return False
            else:
                if abs(b.p[i] - self.p[i]) > 0.01:
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
        self.score = run.run_jobs(100)
        print sum(self.score)
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
        
        crossover_count = 3
        more_bots = []
        for i in range(crossover_count):
            b1 = Bot(None)
            b2 = Bot(None)
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
        for b in bots[:len(bots)/4]:
            if random.random() < mutation_rate*4:
                b = b.clone()
                b.p.mutate()
                bots.append(b)

        bots = uniq(bots)
        for b in bots:
            if b.score == [0]:
                b.run()

        print '-'*30
        bots.sort(key=lambda p: sum(p.score), reverse=True)
        print [sum(b.score) for b in bots]
        print bots[0].score, bots[0].p.to_args()

        for b in bots[:20]:
            print b.p.p, ','
        print

if __name__ == '__main__':
    main()
