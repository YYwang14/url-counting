#!/usr/bin/env python3

def main():
    input_file = "urls.txt"
    output_file = "result.txt"
    
    # Dictionary to store url -> frequency
    url_counts = {}

    # Read and count
    with open(input_file, "r", encoding="utf-8") as f:
        for line in f:
            url = line.strip()
            if url:  # skip empty lines
                url_counts[url] = url_counts.get(url, 0) + 1

    # Sort URLs by frequency (descending)
    # sorted_counts is a list of tuples (url, count)
    sorted_counts = sorted(url_counts.items(), key=lambda x: x[1], reverse=True)

    # Take the top 100
    top_100 = sorted_counts[:100]

    # Write the results to the output file
    with open(output_file, "w", encoding="utf-8") as out:
        out.write("Rank\tURL\tCount\n")
        rank = 1
        for url, count in top_100:
            out.write(f"{rank}\t{url}\t{count}\n")
            rank += 1

if __name__ == "__main__":
    main()
