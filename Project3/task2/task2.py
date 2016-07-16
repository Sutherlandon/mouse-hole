#!/usr/bin/python

import sys

def enum_key(current):
    """Return the next key based on the current key as hex string.

    TODO: Implement the required functions.
    """
    return "Your should implement this function! We are going to test it!"

def main(argv):
    if argv[0] == 'enum_key':
        print (enum_key(argv[1]))
    elif argv[0] == 'crack':
        """TODO: Add your own code and do whatever you do.
        """
    else:
        raise Exception("Wrong mode!")

if __name__=="__main__":
    main(sys.argv[1:])
