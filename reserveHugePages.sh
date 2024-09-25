#! /bin/bash
# The command-line parsing of this script is based on the example from:
# https://stackoverflow.com/questions/192249/how-do-i-parse-command-line-arguments-in-bash/29754866#29754866

printUsageAndExit() {
    echo "Error: please specify the following arguments:"
    echo "  -nX, --node=X     reserve huge pages on memory node X"
    echo "  -lY, --large=Y    reserve Y large (2MB) pages"
    echo "  -hZ, --huge=Z     reserve Z huge (1GB) pages"
    exit -1
}

getopt --test > /dev/null
if (( $? != 4 )); then
    echo "Error: this system has an old version of getopt."
    exit -1
fi

short_options=n:l:h:
long_options=node:,large:,huge:

parsed=$(getopt --options=$short_options \
    --longoptions=$long_options \
    --name "$0" \
    -- "$@")
if (( $? != 0 )); then
    echo "Error: getopt couldn't parse the command-line arguments."
    exit -1
fi
eval set -- "$parsed"

while true; do
    case "$1" in
        -n|--node)
            node="$2"; shift 2;;
        -l|--large)
            large="$2"; shift 2;;
        -h|--huge)
            huge="$2"; shift 2;;
        --)
            shift; break;;
        *)
            printUsageAndExit;;
    esac
done

# all positional (i.e., non optional) arguments are now left in "$@"

if [[ -z $node ]]; then
    # if the user didn't specify the memory node, deduce it from the affinity mask
    node=`numactl -s | grep nodebind | cut -d ' ' -f 2`
    echo "Allocating hugepages on node${node}..."
fi

if [[ -z $large || -z $huge ]]; then
    printUsageAndExit
fi

getLargePagesCount() {
    cat $large_pages_file
}

getHugePagesCount() {
    if [ -f "$huge_pages_file" ]; then
        cat $huge_pages_file
    else
        echo 0
    fi
}

setLargePagesCount() {
    echo "Trying to allocate $1 large pages"
    sudo bash -c "echo $1 > $large_pages_file"
}

setHugePagesCount() {
    if [ -f "$huge_pages_file" ]; then
        echo "Trying to allocate $1 huge pages"
        sudo bash -c "echo $1 > $huge_pages_file"
    else
        echo "WARNING: 1GB hugepages are not supported/enabled"
        echo "WARNING: $huge_pages_file does not exist. Skipping reserving 1GB hugepages"
    fi
}

disableTHP() {
    thp=`cat /sys/kernel/mm/transparent_hugepage/enabled`
    if [[ $thp != "always madvise [never]" ]]; then
        echo "Disable Transparent Huge Papges (set THP to never)"
        sudo bash -c "echo never > /sys/kernel/mm/transparent_hugepage/enabled"
    fi
}

setOvercommitMemory() {
    overcommit=`cat /proc/sys/vm/overcommit_memory`
    if (( $overcommit != 1 )); then
        echo "Enable overcommit memory"
        sudo bash -c "echo 1 > /proc/sys/vm/overcommit_memory"
    fi
}

printHugePagesStatus() {
    echo "  # of 2MB pages(node${node}) == $(getLargePagesCount)"
    echo "  # of 1GB pages(node${node}) == $(getHugePagesCount)"
}

proc_path=/sys/devices/system/node/node${node}/hugepages
large_pages_file=${proc_path}/hugepages-2048kB/nr_hugepages
huge_pages_file=${proc_path}/hugepages-1048576kB/nr_hugepages

echo "---------------- DEBUG INFO ----------------"
setOvercommitMemory
disableTHP
echo "Reserving hugepages..."
echo "Currently:"
printHugePagesStatus

if (( $(getLargePagesCount) > $large )); then
    # first, release large pages to make room for the huge pages
    setLargePagesCount $large
    # hopefully, we will now be able to reserve the requested huge pages
    setHugePagesCount $huge
else
    # first, reserve huge pages, as they require more memory contiguity
    setHugePagesCount $huge
    # second, reserve the large pages
    setLargePagesCount $large
fi

echo "Updated:"
printHugePagesStatus
echo "--------------------------------------------"

if (( $(getHugePagesCount) >= $huge && $(getLargePagesCount) >= $large )); then
    echo "Huge pages were set correctly"
    exit 0
else
    echo "Error: could not reserve the requested number of huge pages. Possible solutions:"
    echo "1. The memory is fragmented. Please reboot the system and try again."
    echo "2. Please check that 1GB pages are supported in your system."
    exit 0
fi

