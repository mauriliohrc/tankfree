"""Crypto unit tests — no hardware, no proprietary data.

Validates the JieLi cipher with synthetic known-answer vectors and round-trips,
so CI can run without the (proprietary) factory/UltraRig firmware present.
"""
import os, sys
sys.path.insert(0, os.path.join(os.path.dirname(__file__), ".."))

from jltech import decrypt_app, encrypt_app, jl_crc16, jl_enc_cipher, CHIP_KEY_TANKG


def test_enc_cipher_known_answer():
    # XOR-stream over zeros reveals the raw keystream for key=0x980F.
    buf = bytearray(8)
    jl_enc_cipher(buf, 0, 8, 0x980F)
    # keystream low-bytes of the LFSR starting at 0x980F
    ks = []
    k = 0x980F
    for _ in range(8):
        ks.append(k & 0xFF)
        k = ((k << 1) ^ (0x1021 if k & 0x8000 else 0)) & 0xFFFF
    assert list(buf) == ks, (list(buf), ks)


def test_app_roundtrip():
    # encrypt∘decrypt == identity for arbitrary data (symmetric XOR stream).
    data = bytes((i * 37 + 11) & 0xFF for i in range(32 * 20 + 7))
    assert encrypt_app(decrypt_app(data)) == data
    assert decrypt_app(encrypt_app(data)) == data


def test_block_keying_changes_per_block():
    # Two identical 32-byte blocks must encrypt differently (address-keyed).
    data = bytes([0xAA]) * 64
    enc = encrypt_app(data, CHIP_KEY_TANKG)
    assert enc[:32] != enc[32:64]


def test_crc16_ccitt():
    # CRC-16/CCITT-FALSE over b"123456789" with init 0 → 0x31C3.
    assert jl_crc16(b"123456789", 0) == 0x31C3


if __name__ == "__main__":
    for name, fn in sorted(globals().items()):
        if name.startswith("test_") and callable(fn):
            fn(); print("ok", name)
    print("ALL PASS")
