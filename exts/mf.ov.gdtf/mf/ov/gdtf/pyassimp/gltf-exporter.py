import sys
import load, export

def main():
    if len(sys.argv) <= 2:
        print("Need at least 2 arguments")
        return
    
    inputFile = sys.argv[1]
    outputFile = sys.argv[2]

    print("Input 3ds file:" + inputFile)
    print("output file: " + outputFile)

    with load('hello.3ds') as scene:
        export(scene, outputFile, "gltf")


if __name__ == "__main__":
    main()

