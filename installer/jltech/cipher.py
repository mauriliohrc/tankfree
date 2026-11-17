"""JieLi cipher/scrambling routines + the AC791N app (SFC) decrypt/encrypt.

Validated: decrypting the real UltraRig app (`m571_g_app.bin`) with
`decrypt_app(..., chip_key=0x980F)` yields plaintext starting with the
`app_area_head` directory and readable firmware strings.
"""

from .crc import jl_crc16

ENC_POLY = 0x1021
CHIP_KEY_TANKG = 0x980F
HEADER_KEY = 0xFFFF
SFC_BLOCK = 32


def jl_enc_cipher(buf: bytearray, off: int, size: int, key: int) -> int:
    """JieLi "ENC" LFSR stream cipher, in place. Returns the evolved key.

    byte ^= key & 0xFF ; key = ((key<<1) ^ (0x1021 if key&0x8000 else 0)) & 0xFFFF
    """
    for i in range(size):
        buf[off + i] ^= key & 0xFF
        key = ((key << 1) ^ (ENC_POLY if key & 0x8000 else 0)) & 0xFFFF
    return key


def jl_crc_cipher(buf: bytearray, off: int, size: int, key: int = 0xFFFFFFFF) -> None:
    """JieLi "CrcDecode" cipher, in place. XORs each byte with a CRC16 advanced
    over the gb2312 magic string, seeded from `key`."""
    magic = "孟黎我爱你，玉林".encode("gb2312")
    crc = jl_crc16((key >> 16).to_bytes(2, "little"), key & 0xFFFF)
    for i in range(size):
        crc = jl_crc16(magic[i % len(magic):i % len(magic) + 1], crc)
        buf[off + i] ^= crc & 0xFF


def _sfc_block_key(block_index: int, chip_key: int) -> int:
    """Per-32-byte-block key: chip_key XOR ((byte_offset >> 2) & 0xFFFF).

    byte_offset = block_index * 32, offset relative to the app-region start.
    """
    byte_off = block_index * SFC_BLOCK
    return (chip_key ^ ((byte_off >> 2) & 0xFFFF)) & 0xFFFF


def decrypt_app(data: bytes, chip_key: int = CHIP_KEY_TANKG) -> bytes:
    """Decrypt an SFC-encrypted app image (e.g. m571_g_app.bin / flash 0x4000)."""
    buf = bytearray(data)
    for bi, off in enumerate(range(0, len(buf), SFC_BLOCK)):
        jl_enc_cipher(buf, off, min(SFC_BLOCK, len(buf) - off), _sfc_block_key(bi, chip_key))
    return bytes(buf)


# The SFC cipher is symmetric (XOR stream), so encrypt == decrypt.
encrypt_app = decrypt_app


def decrypt_header(data: bytes) -> bytes:
    """Decrypt a flash/container header (first blocks) with the fixed 0xFFFF key,
    resetting the key per 32-byte block."""
    buf = bytearray(data)
    for off in range(0, len(buf), SFC_BLOCK):
        jl_enc_cipher(buf, off, min(SFC_BLOCK, len(buf) - off), HEADER_KEY)
    return bytes(buf)
