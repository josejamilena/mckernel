#!/usr/bin/bash

#!/usr/bin/bash -x

MYHOME=/work/gg10/e29005
test_dir=${MYHOME}/project/os/mckernel/test/uti/mpi
mck_dir=${MYHOME}/project/os/install

exe=`basename $0 | sed 's/\.sh//'`

stop=0
reboot=0
go=0

interactive=0
pjsub=0
gdb=0
disable_uti=0
mck=0
nnodes=2
LASTNODE=8196
use_hfi=0
omp_num_threads=3
ppn=4
lpp=4 # logical-per-physical

while getopts srgc:ml:N:P:o:hGdip OPT
do
        case ${OPT} in
            s) stop=1
                ;;
            r) reboot=1
                ;;
	    g) go=1
		;;
            m) mck=1
                ;;
	    N) nnodes=$OPTARG
		;;
	    P) ppn=$OPTARG
		;;
	    o) omp_num_threads=$OPTARG
		;;
	    h) use_hfi=1
		;;
	    G) gdb=1
		;;
	    d) disable_uti=1
		;;
	    i) interactive=1
		;;
	    p) pjsub=1
		;;
            *) echo "invalid option -${OPT}" >&2
                exit 1
        esac
done

nprocs=$((ppn * nnodes))
nodes=`echo $(seq -s ",c" $(($LASTNODE + 1 - $nnodes)) $LASTNODE) | sed 's/^/c/'`
nsamples=1

#uti_cpu_set=64,65,66,67,132,133,134,135,200,201,202,203,268,269,270,271
uti_cpu_set=204-271

if [ $mck -eq 0 ]; then
    uti_cpu_set_str="export UTI_CPU_SET=$uti_cpu_set"
    i_mpi_pin_processor_exclude_list="export I_MPI_PIN_PROCESSOR_EXCLUDE_LIST=$uti_cpu_set"
else
    uti_cpu_set_str=
    i_mpi_pin_processor_exclude_list=
fi

echo uti_cpu_set=$uti_cpu_set

if [ ${mck} -eq 1 ]; then
    i_mpi_pin=off
    i_mpi_pin_domain=
    i_mpi_pin_order=
    kmp_affinity="export KMP_AFFINITY=disabled" # Without this, rank is bound to OMP_NUM_THREAD-sized domain
else
    i_mpi_pin=on
    domain=$omp_num_threads
    i_mpi_pin_domain="export I_MPI_PIN_DOMAIN=$domain"
    i_mpi_pin_order="export I_MPI_PIN_ORDER=compact"
    kmp_affinity="export KMP_AFFINITY=granularity=thread,scatter"
fi

echo nprocs=$nprocs nnodes=$nnodes ppn=$ppn nodes=$nodes nsamples=$nsamples domain=$domain

if [ ${mck} -eq 1 ]; then
    use_mck="#PJM -x MCK=$mck_dir"
    mck_mem="#PJM -x MCK_MEM=32G@0,8G@1"
    mcexec="${mck_dir}/bin/mcexec"
    nmcexecthr=$((omp_num_threads + 4))
    mcexecopt="--enable-uti -n $ppn -t $nmcexecthr $mcexecopt"
    if [ ${use_hfi} -eq 1 ]; then
	mcexecopt="--enable-hfi1 $mcexecopt"
    fi
else
    use_mck=
    mck_mem=
    mcexec=
    mcexecopt=
fi

if [ $gdb -eq 1 ]; then
    enable_x="-enable-x"
    gdbcmd="xterm -display localhost:11 -hold -e gdb -ex run --args"
fi

if [ $disable_uti -eq 1 ]; then
    disable_uti_str="export DISABLE_UTI=1"
else
    disable_uti_str=
fi

if [ $interactive -eq 1 ]; then
    i_mpi_hydra_bootstrap_exec=
    i_mpi_hydra_bootstrap=
    hosts=
    opt_dir=/opt/intel
else
    PDSH_SSH_ARGS_APPEND="-tt -q" pdsh -t 2 -w $nodes bash -c \'if \[ \"\`cat /etc/mtab \| while read line\; do cut -d\" \" -f 2\; done \| grep /work\`\" == \"\" \]\; then sudo mount /work\; fi\'
    i_mpi_hydra_bootstrap_exec="export I_MPI_HYDRA_BOOTSTRAP_EXEC=/usr/bin/ssh"
    i_mpi_hydra_bootstrap="export I_MPI_HYDRA_BOOTSTRAP=ssh"
    hosts="-hosts $nodes"
    opt_dir=/home/opt/local/cores/intel
fi

# If using ssh
if [ $pjsub -eq 0 ] && [ $interactive -eq 0 ]; then
    compilervars=". ${opt_dir}/compilers_and_libraries_2018.1.163/linux/bin/compilervars.sh intel64"
else
    compilervars=
fi

if [ ${stop} -eq 1 ]; then
    if [ ${mck} -eq 1 ]; then
	PDSH_SSH_ARGS_APPEND="-tt -q" pdsh -t 2 -w $nodes \
	    sudo ${mck_dir}/sbin/mcstop+release.sh
    else
	:
    fi
fi

if [ ${reboot} -eq 1 ]; then
    if [ ${mck} -eq 1 ]; then
	if hostname  | grep ofp &>/dev/null; then

#	    PDSH_SSH_ARGS_APPEND="-tt -q" pdsh -t 2 -w $nodes \
#	    sudo ${mck_dir}/sbin/mcreboot.sh -s -c 2-17,70-85,138-153,206-221,20-35,88-103,156-171,224-239,36-51,104-119,172-187,240-255,52-67,120-135,188-203,256-271 -r 2-5,70-73,138-141,206-209:0+6-9,74-77,142-145,210-213:1+10-13,78-81,146-149,214-217:68+14-17,82-85,150-153,218-221:69+20-23,88-91,156-159,224-227:136+24-27,92-95,160-163,228-231:137+28-31,96-99,164-167,232-235:204+32-35,100-103,168-171,236-239:205+36-39,104-107,172-175,240-243:18+40-43,108-111,176-179,244-247:19+44-47,112-115,180-183,248-251:86+48-51,116-119,184-187,252-255:87+52-55,120-123,188-191,256-259:154+56-59,124-127,192-195,260-263:155+60-63,128-131,196-199,264-267:222+64-67,132-135,200-203,268-271:223 -m 32G@0,12G@1

# perl -e 'for ($i=0;$i<68;$i++){if($i>0){print "+";}printf("%d,%d,%d:%d", $i+68,$i+136,$i+204,$i);}'

	    PDSH_SSH_ARGS_APPEND="-tt -q" pdsh -t 2 -w $nodes \
	    sudo ${mck_dir}/sbin/mcreboot.sh -s -c 68-271 -r 68,136,204:0+69,137,205:1+70,138,206:2+71,139,207:3+72,140,208:4+73,141,209:5+74,142,210:6+75,143,211:7+76,144,212:8+77,145,213:9+78,146,214:10+79,147,215:11+80,148,216:12+81,149,217:13+82,150,218:14+83,151,219:15+84,152,220:16+85,153,221:17+86,154,222:18+87,155,223:19+88,156,224:20+89,157,225:21+90,158,226:22+91,159,227:23+92,160,228:24+93,161,229:25+94,162,230:26+95,163,231:27+96,164,232:28+97,165,233:29+98,166,234:30+99,167,235:31+100,168,236:32+101,169,237:33+102,170,238:34+103,171,239:35+104,172,240:36+105,173,241:37+106,174,242:38+107,175,243:39+108,176,244:40+109,177,245:41+110,178,246:42+111,179,247:43+112,180,248:44+113,181,249:45+114,182,250:46+115,183,251:47+116,184,252:48+117,185,253:49+118,186,254:50+119,187,255:51+120,188,256:52+121,189,257:53+122,190,258:54+123,191,259:55+124,192,260:56+125,193,261:57+126,194,262:58+127,195,263:59+128,196,264:60+129,197,265:61+130,198,266:62+131,199,267:63+132,200,268:64+133,201,269:65+134,202,270:66+135,203,271:67 -m 32G@0,12G@1
	else
	    echo "unkwon host type"
	    exit 1
	fi
    else
	:
    fi
fi

cd ${test_dir}
(
cat <<EOF
#!/bin/sh

#PJM -L rscgrp=$rg
#PJM -L node=$nnodes
#PJM --mpi proc=$nprocs
#PJM -L elapse=$elapse
#PJM -L proc-crproc=16384 
#PJM -g gg10
#PJM -j
#PJM -s
$use_mck
$mck_mem

$i_mpi_hydra_bootstrap_exec
$i_mpi_hydra_bootstrap

export OMP_NUM_THREADS=$omp_num_threads
#export OMP_STACKSIZE=64M
export KMP_BLOCKTIME=1
export PSM2_RCVTHREAD=0

$uti_cpu_set_str
$disable_uti_str
export I_MPI_PIN=$i_mpi_pin
$i_mpi_pin_processor_exclude_list
$i_mpi_pin_domain
$i_mpi_pin_order
$kmp_affinity

export HFI_NO_CPUAFFINITY=1
export I_MPI_COLL_INTRANODE_SHM_THRESHOLD=4194304
export I_MPI_FABRICS=shm:tmi
export PSM2_RCVTHREAD=0
export I_MPI_TMI_PROVIDER=psm2
export I_MPI_FALLBACK=0
export PSM2_MQ_RNDV_HFI_WINDOW=4194304
export PSM2_MQ_EAGER_SDMA_SZ=65536
export PSM2_MQ_RNDV_HFI_THRESH=200000

export MCKERNEL_RLIMIT_STACK=32M,16G
export KMP_STACKSIZE=64m
#export KMP_HW_SUBSET=64c,1t

export I_MPI_ASYNC_PROGRESS=off

#export I_MPI_STATS=native:20,ipm
#export I_MPI_STATS=ipm
#export I_MPI_DEBUG=4
#export I_MPI_HYDRA_DEBUG=on

$compilervars
mpiexec.hydra -n $nprocs -ppn $ppn $hosts $ilpopt $enable_x $gdbcmd $mcexec $mcexecopt ${test_dir}/$exe -n $nsamples -p $ppn
#-l

EOF
) > ./job.sh
chmod u+x ./job.sh

if [ ${go} -eq 1 ]; then
    if [ $pjsub -eq 1 ]; then
	pjsub ./job.sh
    else
	. ${opt_dir}/compilers_and_libraries_2018.1.163/linux/bin/compilervars.sh intel64
	cd ${test_dir}
	make ./$exe
	ssh c$LASTNODE ${test_dir}/job.sh
    fi
fi
