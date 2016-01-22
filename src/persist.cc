// This file is part of Milton.
//
// Milton is free software: you can redistribute it and/or modify it under the
// terms of the GNU Lesser General Public License as published by the Free
// Software Foundation, either version 3 of the License, or (at your option)
// any later version.
//
// Milton is distributed in the hope that it will be useful, but WITHOUT ANY
// WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
// FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for
// more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with Milton.  If not, see <http://www.gnu.org/licenses/>.


#define MILTON_MAGIC_NUMBER 0X11DECAF3

static u32 word_swap_memory_order(u32 word)
{
    u8 llo = (u8) (word & 0x000000ff);
    u8 lhi = (u8)((word & 0x0000ff00) >> 8);
    u8 hlo = (u8)((word & 0x00ff0000) >> 16);
    u8 hhi = (u8)((word & 0xff000000) >> 24);
    return
            (u32)(llo) << 24 |
            (u32)(lhi) << 16 |
            (u32)(hlo) << 8  |
            (u32)(hhi);
}

void milton_load(MiltonState* milton_state)
{
    FILE* fd = fopen("MiltonPersist.mlt", "rb");
    b32 valid = true;
    if ( fd ) {
        u32 milton_magic = (u32)-1;
        fread(&milton_magic, sizeof(u32), 1, fd);
        u32 milton_binary_version = (u32)-1;
        fread(&milton_binary_version, sizeof(u32), 1, fd);

        assert (milton_binary_version == 1);

        fread(milton_state->view, sizeof(CanvasView), 1, fd);

        milton_magic = word_swap_memory_order(milton_magic);

        if ( milton_magic != MILTON_MAGIC_NUMBER ) {
            assert (!"Magic number not found");
            valid = false;
        }

        if (valid) {
            i32 num_strokes = -1;
            fread(&num_strokes, sizeof(i32), 1, fd);

            assert (num_strokes >= 0);

            for ( i32 stroke_i = 0; stroke_i < num_strokes; ++stroke_i ) {
                push(milton_state->strokes, {});
                Stroke* stroke = &milton_state->strokes[milton_state->strokes.count - 1];
                fread(&stroke->brush, sizeof(Brush), 1, fd);
                fread(&stroke->num_points, sizeof(i32), 1, fd);
                if ( stroke->num_points >= STROKE_MAX_POINTS || stroke->num_points <= 0 ) {
                    milton_log("WTF: File has a stroke with %d points\n", stroke->num_points);
                    // Corrupt file. Avoid this read
                    continue;       // Do not allocate, just move on.
                }
                // TODO: Loading a large drawing will result in many calloc
                // calls that can be reduced in various ways. Check if this is a problem.
                stroke->pressures = (f32*)mlt_calloc((size_t)stroke->num_points, sizeof(f32));
                stroke->points    = (v2i*)mlt_calloc((size_t)stroke->num_points, sizeof(v2i));

                fread(stroke->points, sizeof(v2i), (size_t)stroke->num_points, fd);
                fread(stroke->pressures, sizeof(f32), (size_t)stroke->num_points, fd);
            }

            fread(&milton_state->gui->picker.info, sizeof(PickerData), 1, fd);
        }
        fclose(fd);
    }
}

// TODO: Robust saving.
//  - Save to temp file and swap!
//  - Check all stdio function return values
void milton_save(MiltonState* milton_state)
{
    size_t num_strokes = milton_state->strokes.count;
    StrokeCord strokes = milton_state->strokes;
    FILE* fd = fopen("MiltonPersist.mlt", "wb");

    if (!fd) {
        assert (!"Could not create file");
        return;
    }

    // Note: assuming little-endian.
    u32 milton_magic = word_swap_memory_order(MILTON_MAGIC_NUMBER);

    fwrite(&milton_magic, sizeof(u32), 1, fd);

    u32 milton_binary_version = 1;
    fwrite(&milton_binary_version, sizeof(u32), 1, fd);

    fwrite(milton_state->view, sizeof(CanvasView), 1, fd);

    fwrite(&num_strokes, sizeof(i32), 1, fd);

    for (size_t stroke_i = 0; stroke_i < num_strokes; ++stroke_i) {
        Stroke* stroke = &strokes[stroke_i];
        assert(stroke->num_points > 0);
        fwrite(&stroke->brush, sizeof(Brush), 1, fd);
        fwrite(&stroke->num_points, sizeof(i32), 1, fd);
        fwrite(stroke->points, sizeof(v2i), (size_t)stroke->num_points, fd);
        fwrite(stroke->pressures, sizeof(f32), (size_t)stroke->num_points, fd);
    }

    fwrite(&milton_state->gui->picker.info, sizeof(PickerData), 1, fd);

    fclose(fd);
}

void milton_save_buffer_to_file(MiltonState* milton_state, u8* buffer, i32 w, i32 h)
{

}
