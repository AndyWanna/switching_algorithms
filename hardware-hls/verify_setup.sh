#!/bin/bash
echo "==================================================================="
echo "SW-QPS HLS Setup Verification"
echo "==================================================================="
echo ""

echo "Checking testbenches..."
for tb in tb_sw_qps_uniform.cpp tb_sw_qps_diagonal.cpp tb_sw_qps_quasi_diagonal.cpp tb_sw_qps_log_diagonal.cpp; do
    if [ -f "tb/$tb" ]; then
        echo "  ✓ tb/$tb"
    else
        echo "  ✗ tb/$tb MISSING"
    fi
done

echo ""
echo "Checking source directories..."
if [ -d "src" ]; then
    echo "  ✓ src/ ($(ls src/*.h src/*.cpp 2>/dev/null | wc -l) files)"
else
    echo "  ✗ src/ MISSING"
fi

if [ -d "src_aggressive" ]; then
    echo "  ✓ src_aggressive/ ($(ls src_aggressive/*.h src_aggressive/*.cpp 2>/dev/null | wc -l) files)"
else
    echo "  ✗ src_aggressive/ MISSING"
fi

echo ""
echo "Checking TCL scripts..."
count=0
for tcl in run_sw_qps_*.tcl; do
    if [ -f "$tcl" ]; then
        echo "  ✓ $tcl"
        ((count++))
    fi
done
echo "  Total: $count/8 scripts"

echo ""
echo "Checking Python runner..."
if [ -f "run_parallel_hls.py" ]; then
    echo "  ✓ run_parallel_hls.py"
    if [ -x "run_parallel_hls.py" ]; then
        echo "  ✓ Executable"
    else
        echo "  ⚠ Not executable (chmod +x run_parallel_hls.py)"
    fi
else
    echo "  ✗ run_parallel_hls.py MISSING"
fi

echo ""
echo "==================================================================="
if [ $count -eq 8 ]; then
    echo "✓ Setup complete! Ready to run: python3 run_parallel_hls.py"
else
    echo "⚠ Setup incomplete. Please check missing files."
fi
echo "==================================================================="
