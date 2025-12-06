<AutoPilot:project xmlns:AutoPilot="com.autoesl.autopilot.project" top="sw_qps_top" name="sw_qps_project" ideType="classic">
    <files>
        <file name="src_aggressive/sw_qps_types.h" sc="0" tb="false" cflags="" csimflags="" blackbox="false"/>
        <file name="src_aggressive/utils.h" sc="0" tb="false" cflags="" csimflags="" blackbox="false"/>
        <file name="src_aggressive/qps_sampler.cpp" sc="0" tb="false" cflags="" csimflags="" blackbox="false"/>
        <file name="src_aggressive/output_port.h" sc="0" tb="false" cflags="" csimflags="" blackbox="false"/>
        <file name="src_aggressive/input_port.h" sc="0" tb="false" cflags="" csimflags="" blackbox="false"/>
        <file name="src_aggressive/sliding_window.h" sc="0" tb="false" cflags="" csimflags="" blackbox="false"/>
        <file name="src_aggressive/sw_qps_top.cpp" sc="0" tb="false" cflags="" csimflags="" blackbox="false"/>
        <file name="../../tb/tb_sw_qps_pure.cpp" sc="0" tb="1" cflags="-std=c++11 -DSW_QPS_PURE_DISABLE_MAIN -Wno-unknown-pragmas" csimflags="" blackbox="false"/>
        <file name="../../tb/tb_sw_qps_hls.cpp" sc="0" tb="1" cflags="-Wno-unknown-pragmas" csimflags="" blackbox="false"/>
    </files>
    <solutions>
        <solution name="solution-aggressive" status=""/>
    </solutions>
    <Simulation argv="">
        <SimFlow name="csim" setup="false" optimizeCompile="false" clean="true" ldflags="" mflags=""/>
    </Simulation>
</AutoPilot:project>

