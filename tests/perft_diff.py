import subprocess
import re

# ---------------- CONFIG ----------------

ENGINE_PATH = "/your/engine/build/path"
STOCKFISH_PATH = "/path/to/stockfish/executable"

FEN = "rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8  "
DEPTH = 5

# ----------------------------------------


def run_engine_perft():
    process = subprocess.Popen(
        [ENGINE_PATH, FEN, str(DEPTH)],
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True
    )

    out, err = process.communicate()

    if err:
        print("Šaherator stderr:")
        print(err)

    return parse_divide_output(out)


def run_stockfish_perft():
    """
    Uses Stockfish UCI:
        position fen ...
        go perft N
    """

    process = subprocess.Popen(
        [STOCKFISH_PATH],
        stdin=subprocess.PIPE,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True
    )

    commands = f"""
uci
isready
position fen {FEN}
go perft {DEPTH}
quit
"""

    out, err = process.communicate(commands)

    if err:
        print("Stockfish stderr:")
        print(err)

    return parse_divide_output(out)


def parse_divide_output(output):
    """
    Parses lines like:

    e2e4: 20
    e1g1: 43
    """

    results = {}

    pattern = re.compile(r"([a-h][1-8][a-h][1-8][qrbn]?)\s*:\s*(\d+)")

    for line in output.splitlines():
        match = pattern.search(line)
        if match:
            move = match.group(1)
            nodes = int(match.group(2))
            results[move] = nodes

    return results


def compare_results(ours, stockfish):
    print("\n=== DIFF REPORT ===\n")

    all_moves = sorted(set(ours.keys()) | set(stockfish.keys()))

    mismatch_found = False

    for move in all_moves:
        our_nodes = ours.get(move)
        sf_nodes = stockfish.get(move)

        if our_nodes != sf_nodes:
            mismatch_found = True
            print(
                f"Mismatch on {move}: "
                f"Šaherator={our_nodes}, stockfish={sf_nodes}"
            )

    if not mismatch_found:
        print("✅ Perfect match.")
        return

    print("\nFirst mismatch found above.")
    print("Run deeper divide on that move branch next.")


def main():
    print("Running Šaherator...")
    ours = run_engine_perft()

    print("Running Stockfish...")
    sf = run_stockfish_perft()

    compare_results(ours, sf)


if __name__ == "__main__":
    main()
