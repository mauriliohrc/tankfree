"""JieLi CRC routines (CRC-16-CCITT and CRC-32), pure Python."""


def jl_crc16(data: bytes, crc: int = 0) -> int:
    """CRC-16-CCITT, poly 0x1021, non-reflected, init 0 (default)."""
    for b in data:
        crc ^= b << 8
        for _ in range(8):
            crc = ((crc << 1) ^ 0x1021) & 0xFFFF if crc & 0x8000 else (crc << 1) & 0xFFFF
    return crc & 0xFFFF


def jl_crc32(data: bytes, crc: int = 0x264ED660) -> int:
    """JieLi CRC-32: poly 0x04C11DB7, reflected. init matches jltech default.

    (crcmod.mkCrcFun(0x104C11DB7, initCrc=0x264ED660, rev=True))
    """
    poly = 0xEDB88320  # reflected 0x04C11DB7
    crc ^= 0xFFFFFFFF  # xorOut/xorIn handling to mirror crcmod's rev=True form
    for b in data:
        crc ^= b
        for _ in range(8):
            crc = (crc >> 1) ^ poly if crc & 1 else crc >> 1
    return (crc ^ 0xFFFFFFFF) & 0xFFFFFFFF


# Standard zlib-compatible CRC32 (used by the A2L1 model format / nam2chip).
def crc32_zlib(data: bytes) -> int:
    import zlib
    return zlib.crc32(data) & 0xFFFFFFFF
