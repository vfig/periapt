# This signature seems pretty uniquely to point to cam_render_scene
# quite honestly. If I want to, I could try scanning the entire
# TEXT section for this signature (aligned to 16 bytes)
import sys
filename = sys.argv[1]
window = b''
with open(filename, 'rb') as f:
    while True:
        ofs = f.tell() - 16
        try:
            buf = f.read(16)
        except EOFError:
            break
        if not buf:
            break
        window = window[-16:] + buf
        if len(window) == 32:
            match = True
            match = match and (window[0:5] == b'\x83\xec\x28\xdd\x05')
            # 5:9 - \xd8\xee\x38\x00 (address will vary, and is fixed up)
            match = match and (window[9:17] == b'\x53\x56\xdd\x54\x24\x24\xdd\x05')
            # 17:21 - \xe0\xee\x38\x00 (address will vary, and is fixed up)
            match = match and (window[21:31] == b'\x8b\xf0\x8b\x06\xdd\x54\x24\x1c\xd9\x05')
            if match:
                print("Match at 0x%08x" % ofs)
