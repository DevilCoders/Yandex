import sys

if __name__ == "__main__":
    if len(sys.argv) != 2:
        print("Usage: {} <script>".format(sys.argv[0]))
        sys.exit(1)

    script = sys.argv[1]
    execfile(script)
