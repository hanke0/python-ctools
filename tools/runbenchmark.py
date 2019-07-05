#!/usr/bin/env python
import glob
import os
import argparse
import timeit
import sys
import inspect


def benchmark_setup(**kwargs):
    def decorator(user_func):
        user_func.__setup__ = kwargs
        return user_func

    return decorator


def mean_std(seq):
    mean = sum(seq) / len(seq)
    return mean, sum((x - mean) ** 2 for x in seq) ** 0.5


def humanize(t):
    if t > 1:
        return "%.3f s" % t
    t *= 1000
    if t > 1:
        return "%.3f ms" % t
    t *= 1000
    if t > 1:
        return "%.3f µs" % t
    t *= 1000
    return "%.3f ns" % t


class Benchmark:

    def __init__(self, path):
        self.path = os.path.abspath(path)

        self.files = glob.glob(os.path.join(path, "**/benchmark_*.py"), recursive=True)
        self.rv = {}
        self.finish = False

    def _exec_file(self, file):
        with open(file, "rt") as f:
            s = f.read()
        g = {"benchmark_setup": benchmark_setup}
        exec(s, g)
        return g

    def _is_benchmark(self, name, obj):
        if not inspect.isfunction(obj):
            return False
        if name == 'benchmark_setup':
            return False
        if name.startswith("benchmark_"):
            return True

    def _filter_benchmarks(self, g):
        rv = []
        for name, obj in g.items():
            if self._is_benchmark(name, obj):
                rv.append(name)
        return rv

    def start(self):
        print("rootdir:", self.path)
        print("Collect %d files" % len(self.files), flush=True)
        for file in self.files:
            g = self._exec_file(file)
            benchmarks = self._filter_benchmarks(g)
            print("\nFound {} benchmarks in file {}".format(len(benchmarks), file), flush=True)
            for name in benchmarks:
                b_rv = self.run_str(name, g)
                s = "{}::{}    {}".format(file, name, self._pretty_rv(*b_rv))
                print(s, flush=True)

    def run_str(self, name, g):
        call = g[name]
        args = inspect.signature(call).parameters.keys()
        callstr = "{}({})".format(name, ",".join(args))
        lines = []
        for k, v in getattr(call, "__setup__", {}).items():
            lines.append('{}={}()'.format(k, v.__name__))

        setup = (";".join(lines))

        repeat = 10
        total_cost = 8
        cost = sum(timeit.repeat(callstr, setup=setup, globals=g, number=1, repeat=1))
        if cost > total_cost:
            loop = 3
        else:
            num = int(round(total_cost / cost))
            loop = num // repeat
            if loop <= 3:
                loop = 3
            else:
                count = 10
                while count < loop:
                    count *= 10
                loop = count

        costs = timeit.repeat(callstr, setup=setup, globals=g, number=loop, repeat=repeat)
        costs = [t / loop for t in costs]  # seconds
        return loop, repeat, costs

    @staticmethod
    def _pretty_rv(loop, repeat, costs):
        mean, std = mean_std(costs)
        return "{} ± {}\t(each {:,} runs, {:,} loops)".format(humanize(mean), humanize(std),
                                                              repeat, loop)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("path", default=".")
    if len(sys.argv) != 2:
        path = "."
    else:
        path = sys.argv[1]
        if path.startswith('-'):
            parser.print_help()
            sys.exit(1)

    benchmark = Benchmark(path)

    benchmark.start()
    sys.exit(0)


if __name__ == '__main__':
    main()
