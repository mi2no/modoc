from fontTools.ttLib import TTFont

font = TTFont('src/font/JetBrainsMono/static/JetBrainsMono-Regular.ttf')

# ============================================================
# CMAP — sprawdzanie znaków
# ============================================================

cmap = font.getBestCmap()

chars_to_check = [
    '#', 'i', 'n', 'c', 'l', 'u', 'd', 'e',
    ' ', '<', '>', '\t', '\r'
]

print("--- Character -> Glyph -> GID ---")

for char in chars_to_check:
    cp = ord(char)

    if cp in cmap:
        glyph_name = cmap[cp]
        gid = font.getGlyphID(glyph_name)

        print(
            f"'{char}' (U+{cp:04X}) -> "
            f"{glyph_name} -> "
            f"GID {gid} (0x{gid:04X})"
        )
    else:
        print(f"'{char}' (U+{cp:04X}) -> NOT IN CMAP")


# ============================================================
# Reverse lookup — GID -> Unicode
# ============================================================

print("\n--- Reverse: GID -> Unicode ---")

test_gids = [
    0x0251,
    0x0256,
    0x024B,
    0x02C0,
    0xAA08
]

reverse_cmap = {}

for cp, glyph_name in cmap.items():
    gid = font.getGlyphID(glyph_name)
    reverse_cmap.setdefault(gid, []).append(cp)

for gid in test_gids:
    if gid in reverse_cmap:
        for cp in reverse_cmap[gid]:
            char = chr(cp) if cp < 128 else '?'

            print(
                f"GID 0x{gid:04X} -> "
                f"U+{cp:04X} ('{char}')"
            )
    else:
        print(f"GID 0x{gid:04X} -> NOT IN REVERSE CMAP")


# ============================================================
# CMAP Format 4 — idRangeOffset
# ============================================================

print("\n--- CMAP Format 4 ---")

cmap_table = None

for table in font['cmap'].tables:
    if table.format == 4:
        cmap_table = table
        break

if cmap_table is None:
    print("Nie znaleziono cmap Format 4")

else:
    print("Format:", cmap_table.format)
    print("segCount:", len(cmap_table.endCode))

    print("\nendCode:")
    print(cmap_table.endCode)

    print("\nstartCode:")
    print(cmap_table.startCode)

    print("\nidDelta:")
    print(cmap_table.idDelta)

    print("\nidRangeOffset:")

    for i, offset in enumerate(cmap_table.idRangeOffset):
        print(
            f"[{i:3}] = {offset} "
            f"(0x{offset:04X})"
        )