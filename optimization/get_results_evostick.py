#!/usr/bin/python3

import os
import sys
import re
import shutil

def main():
    t = re.compile('%s-(\d+)' % sys.argv[1])
    os.mkdir('results-evo-%s' % sys.argv[1])
    for i in os.listdir():
        if t.match(i):
            y = t.match(i)
            print(i)
            shutil.copyfile( i+"/gen/gen_last_1_champ",'results-evo-%s/gen_champ_%s' % (sys.argv[1],y.group(1)) )

    #f2.close()


if __name__ == "__main__":
    main()
