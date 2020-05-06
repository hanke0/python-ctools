#!/usr/bin/env python
"""
A benchmark runner.

Files and functions that match pattern 'benchmark_*' is executed as a benchmark.

Use decorator `benchmark_setup` to run setup before run benchmark.

A typical benchmark example:
```
@benchmark_setup(hack_into=int)
def benchmark_print_int(hack_into):
    for i in range(100):
        print(hack_info)
```


"""
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
    def __init__(self, dirs, error_exit=False):
        self.files = []
        for i in dirs:
            self.files.extend(
                glob.glob(os.path.join(i, "**/benchmark_*.py"), recursive=True)
            )

        self.rv = {}
        self.finish = False
        self.error_exit = error_exit

    def _exec_file(self, file):
        with open(file, "rt") as f:
            s = f.read()
        g = {"benchmark_setup": benchmark_setup}
        try:
            exec(s, g)
        except Exception as e:
            sys.stderr.write("Error load file ")
            sys.stderr.write(file)
            sys.stderr.write(": ")
            sys.stderr.write(repr(e))
            sys.stderr.write("\n")
            sys.stderr.flush()
            if self.error_exit:
                sys.exit(1)
            return None
        return g

    @staticmethod
    def _is_benchmark(name, obj):
        if not inspect.isfunction(obj):
            return False
        if name == "benchmark_setup":
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
        print("Total %d files\n" % len(self.files), flush=True)
        for idx, file in enumerate(self.files):
            g = self._exec_file(file)
            if g is None:
                continue

            benchmarks = self._filter_benchmarks(g)
            title = "Collect {} benchmarks in {}".format(len(benchmarks), file)
            print(title, flush=True)
            print("-" * len(title), flush=True)
            for name in benchmarks:
                try:
                    b_rv = self.run_str(name, g)
                except Exception as e:
                    sys.stderr.write("Error run benchmark '")
                    sys.stderr.write(name)
                    sys.stderr.write("' in file ")
                    sys.stderr.write(file)
                    sys.stderr.write(": ")
                    sys.stderr.write(repr(e))
                    sys.stderr.write("\n")
                    sys.stderr.flush()
                    if self.error_exit:
                        sys.exit(1)
                    else:
                        continue

                s = "* {}::{}    {}".format(file, name, self._pretty_rv(*b_rv))
                print(s, flush=True)
            print()

    def run_str(self, name, g):
        call = g[name]
        args = inspect.signature(call).parameters.keys()
        callstr = "{}({})".format(name, ",".join(args))
        lines = []
        for k, v in getattr(call, "__setup__", {}).items():
            lines.append("{}={}()".format(k, v.__name__))

        setup = ";".join(lines)

        repeat = 1
        max_cost = 8
        mask = 20
        cost = sum(timeit.repeat(callstr, setup=setup, globals=g, number=1, repeat=1))
        if cost >= max_cost:
            loop = 1
        elif cost * mask >= max_cost:
            loop = int(round(max_cost / cost))
            loop += loop % 10
        else:
            num = int(round(max_cost / cost))
            while num > mask * repeat and repeat < 10:
                repeat += 1

            loop = num // repeat
            loop += loop % 10

        costs = timeit.repeat(
            callstr, setup=setup, globals=g, number=loop, repeat=repeat
        )
        costs = [t / loop for t in costs]  # seconds
        return loop, repeat, costs

    @staticmethod
    def _pretty_rv(loop, repeat, costs):
        mean, std = mean_std(costs)
        return "{} ± {}\t({:,} loops, each {:,} runs)".format(
            humanize(mean), humanize(std), loop, repeat
        )


def main():
    parser = argparse.ArgumentParser(
        prog=os.path.basename(sys.argv[0]), description=__doc__.lstrip().splitlines()[0]
    )
    parser.add_argument("path", default=["."], nargs="+")
    parser.add_argument(
        "-p",
        "--python-path",
        default=["."],
        nargs="*",
        help="paths inserted to sys.path",
    )
    parser.add_argument(
        "-e",
        "--error-exit",
        default=False,
        action="store_true",
        help="exit if error occurred",
    )

    args = parser.parse_args()
    path = args.path
    py_path = args.python_path
    error_exit = args.error_exit
    if py_path:
        for i in py_path:
            sys.path.insert(0, i)

    benchmark = Benchmark(path, error_exit)
    benchmark.start()

    sys.exit(0)


if __name__ == "__main__":
    main()
