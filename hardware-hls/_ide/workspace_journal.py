# 2025-11-24T00:25:07.335723
import vitis

client = vitis.create_client()
client.set_workspace(path="hardware-hls")

comp = client.create_hls_component(name = "hls_component",cfg_file = ["hls_config.cfg"],template = "empty_hls_component")

comp = client.get_component(name="hls_component")
comp.run(operation="SYNTHESIS")

comp.run(operation="IMPLEMENTATION")

vitis.dispose()

