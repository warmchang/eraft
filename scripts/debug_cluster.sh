docker run -it \
                --net eraft-network \
                --name eraft-1 \
                --hostname eraft-1 \
                --ip 172.19.0.11 \
                -v ${PWD}:/eraft \
                -w /eraft \
                -p 20160:20160 \
                eraft/eraft_pmem:v2


docker run -it \
                --net eraft-network \
                --name eraft-2 \
                --hostname eraft-2 \
                --ip 172.19.0.12 \
                -v ${PWD}:/eraft \
                -p 20161:20161 \
                eraft/eraft_pmem:v2


docker run -it \
                --net eraft-network \
                --name eraft-3 \
                --hostname eraft-3 \
                --ip 172.19.0.13 \
                -v ${PWD}:/eraft \
                -p 20162:20162 \
                eraft/eraft_pmem:v2