#!/usr/bin/env bash
set -euo pipefail

# ---------- defaults ----------
CGROUP_PATH="/sys/fs/cgroup/disk_mdtrie"
MEMORY_LIMIT="infinity"
INTERVAL=0.005              # seconds
# ------------------------------

# --- parse --mem and --interval flags (order-agnostic) ---
while [[ "$#" -gt 0 && "$1" == --* ]]; do
  case "$1" in
    --mem=*)      MEMORY_LIMIT="${1#--mem=}" ;;
    --interval=*) INTERVAL="${1#--interval=}" ;;
    --help) echo "Usage: $0 [--mem=<limit>] [--interval=<sec>] command..."; exit 0 ;;
    *) echo "Unknown flag: $1" >&2; exit 1 ;;
  esac
  shift
done

# Remaining args are the command
[[ "$#" -ge 1 ]] || { echo "Usage: $0 [--mem=<limit>] command..."; exit 1; }
CMD=("$@")

# --- extract benchmark tag (-b foo or -bfoo) ---
BENCH_TAG=cmd
for ((i=0; i<${#CMD[@]}; i++)); do
  arg="${CMD[i]}"
  if [[ "$arg" == "-b" && $((i+1)) -lt ${#CMD[@]} ]]; then
    BENCH_TAG="${CMD[i+1]}"; break
  elif [[ "$arg" == -b* ]]; then
    BENCH_TAG="${arg#-b}"; break
  fi
done

# sanitise tags for filenames
mem_tag="${MEMORY_LIMIT//[^0-9A-Za-z]/}"
bench_tag="${BENCH_TAG//[^0-9A-Za-z_-]/}"
timestamp="$(date +%Y%m%d_%H%M%S)"

LOGFILE="memlog_${bench_tag}_${mem_tag}.log"

# --- cgroup setup ---
sudo mkdir -p "$CGROUP_PATH"
echo "${MEMORY_LIMIT/infinity/max}" | sudo tee "$CGROUP_PATH/memory.max" >/dev/null
echo 0 | sudo tee "$CGROUP_PATH/memory.swap.max" >/dev/null

# --- launch & monitor ---
"${CMD[@]}" &
APP_PID=$!
echo "$APP_PID" | sudo tee "$CGROUP_PATH/cgroup.procs" >/dev/null

{
  echo "# Command: ${CMD[*]}"
  echo "# Memory limit: $MEMORY_LIMIT"
  echo "# Interval: ${INTERVAL}s"
  echo "# Timestamp    Total(B)   Anon(B)    File(B)   pgfault   pgmajfault"
} > "$LOGFILE"

while kill -0 "$APP_PID" 2>/dev/null; do
  ts=$(date +%s)
  mem_total=$(<"$CGROUP_PATH/memory.current")
  read -r _ anon  < <(grep '^anon '  "$CGROUP_PATH/memory.stat")
  read -r _ file  < <(grep '^file '  "$CGROUP_PATH/memory.stat")
  read -r _ pgf   < <(grep '^pgfault '   "$CGROUP_PATH/memory.stat")
  read -r _ pgmaj < <(grep '^pgmajfault ' "$CGROUP_PATH/memory.stat")

  printf "%d %12d %10d %10d %9d %11d\n" \
          "$ts" "$mem_total" "$anon" "$file" "$pgf" "$pgmaj" >> "$LOGFILE"

  sleep "$INTERVAL"
done

echo "# Process finished at $(date)" >> "$LOGFILE"