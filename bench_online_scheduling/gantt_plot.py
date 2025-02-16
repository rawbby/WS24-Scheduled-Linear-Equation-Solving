import glob
import os
import re
import sys

import matplotlib as mpl
import matplotlib.pyplot as plt
import numpy as np


def process_file(filename):
    """Process a single file using np.memmap and return its thread id and tasks array."""
    tid = int(re.search(r"^t(\d+)_", filename).group(1))
    filesize = os.path.getsize(filename)
    n_records = filesize // 24  # each record is 24 bytes (3 uint64 values)
    # Create a memmap of the file; note that each record has three little-endian uint64 values.
    arr = np.memmap(filename, dtype='<Q', mode='r', shape=(n_records, 3))

    # Get baseline time from the first record.
    x0 = arr[0, 1] * 1e-9
    w0 = arr[0, 2] * 1e-9

    # Allocate a tasks array: each row is (start_time, width)
    tasks = np.empty((n_records, 2), dtype=np.float64)
    tasks[0, 0] = 0.0
    tasks[0, 1] = w0
    tasks[1:, 0] = arr[1:, 1] * 1e-9 - x0
    tasks[1:, 1] = arr[1:, 2] * 1e-9

    # Optionally delete the memmap reference if not needed further.
    del arr
    return tid, tasks


def gen_x_segments(tid, tasks, h):
    """Generate x and y segments for plotting borders using vectorized NumPy operations."""
    n = tasks.shape[0]
    a_vals = tasks[:, 0]
    # For each task, create three values: [start, start, nan]
    repeated = np.empty((n, 3), dtype=np.float64)
    repeated[:, 0] = a_vals
    repeated[:, 1] = a_vals
    repeated[:, 2] = np.nan
    x_segments = repeated.ravel()
    # Append final segment from the last task's end.
    x_last_val = tasks[-1, 0] + tasks[-1, 1]
    x_last = np.array([x_last_val, x_last_val, np.nan], dtype=np.float64)
    x_segments = np.concatenate([x_segments, x_last])

    # Create y segments for this thread.
    m = n + 1
    y_base = tid - h / 2
    y_arr = np.empty((m, 3), dtype=np.float64)
    y_arr[:, 0] = y_base
    y_arr[:, 1] = y_base + h
    y_arr[:, 2] = np.nan
    y_segments = y_arr.ravel()
    return x_segments, y_segments


def main():
    suffix = sys.argv[1]
    files = sorted(
        glob.glob(f"t*_{suffix}.dump"),
        key=lambda f: int(re.search(r"^t(\d+)_", f).group(1))
    )
    num_threads = len(files)
    h = 0.3
    dpi = 300

    mpl.rcParams['agg.path.chunksize'] = 25000
    fig, ax = plt.subplots(figsize=(16, 3 + 1.5 * h * num_threads))

    dark_blue = [0x00 / 255, 0x77 / 255, 0xb6 / 255]
    light_blue = [0x48 / 255, 0xca / 255, 0xe4 / 255]
    pixel = 72 / dpi
    linewidth = 2 * pixel

    # Process each file sequentially.
    for filename in files:
        tid, tasks = process_file(filename)
        print(f"Loaded Data for Thread {tid}")

        # Plot the broken bars for this thread.
        y = tid - h / 2
        ax.broken_barh(tasks.tolist(), (y, h), facecolors=light_blue,
                       edgecolor=light_blue, linewidth=linewidth)

        # Generate and plot borders.
        x_seg, y_seg = gen_x_segments(tid, tasks, h)
        ax.plot(x_seg, y_seg, color=dark_blue, linewidth=linewidth)

    ax.set_xlabel("Time (Seconds)")
    ax.set_yticks(list(range(num_threads)))
    ax.set_yticklabels([f"Thread {i}" for i in range(num_threads)])
    ax.set_title(f"Gantt Chart for `{suffix}` Scheduler")

    print("Rendering...")
    plt.tight_layout()
    plt.savefig(f"gantt_{suffix}.png", dpi=dpi)


if __name__ == '__main__':
    main()
