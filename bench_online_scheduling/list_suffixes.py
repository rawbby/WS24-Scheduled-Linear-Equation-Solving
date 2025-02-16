import glob
import re


def main():
    suffixes = []
    for file in glob.glob(f"t*.dump"):
        suffix = re.search(r"^t\d+_(.*)\.dump$", file).group(1)
        if suffix not in suffixes:
            suffixes.append(suffix)
    for suffix in sorted(suffixes):
        print(suffix)


if __name__ == '__main__':
    main()
