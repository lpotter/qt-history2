#include <private/qdrawhelper_p.h>

#if 1//def __x86__
#include "qdrawhelper_x86.cpp"
#else
void qInitAsm(DrawHelper *) {}
#endif

static void blend_color(ARGB *target, const QSpan *span, ARGB color)
{
    if (!span->len)
        return;

    int alpha = qt_div_255(color.a * span->coverage);
    int pr = alpha * color.r;
    int pg = alpha * color.g;
    int pb = alpha * color.b;

    int rev_alpha = 255 - alpha;

    for (int i = span->len; i > 0 ; --i) {
        qt_alpha_pixel_pm(pr, target->r, rev_alpha);
        qt_alpha_pixel_pm(pg, target->g, rev_alpha);
        qt_alpha_pixel_pm(pb, target->b, rev_alpha);
        target->a = 255;
        ++target;
    }
}

static void blend_transformed_bilinear(ARGB *target, const QSpan *span, qreal ix, qreal iy, qreal dx, qreal dy,
                                       ARGB *image_bits, int image_width, int image_height)
{
    const int fixed_scale = 1 << 16;
    int x = int((ix + dx * span->x) * fixed_scale);
    int y = int((iy + dy * span->x) * fixed_scale);

    int fdx = (int)(dx * fixed_scale);
    int fdy = (int)(dy * fixed_scale);

    for (int i = 0; i < span->len; ++i) {
        int x1 = (x >> 16);
        int x2 = x1 + 1;
        int y1 = (y >> 16);
        int y2 = y1 + 1;

        int distx = ((x - (x1 << 16)) >> 8);
        int disty = ((y - (y1 << 16)) >> 8);
        int idistx = 256 - distx;
        int idisty = 256 - disty;

        bool x1_out = ((x1 < 0) | (x1 >= image_width));
        bool x2_out = ((x2 < 0) | (x2 >= image_width));
        bool y1_out = ((y1 < 0) | (y1 >= image_height));
        bool y2_out = ((y2 < 0) | (y2 >= image_height));

        int y1_offset = y1 * image_width;
        int y2_offset = y1_offset + image_width;

        ARGB tl = (x1_out | y1_out) ? 0 : image_bits[y1_offset + x1];
        ARGB tr = (x2_out | y1_out) ? 0 : image_bits[y1_offset + x2];
        ARGB bl = (x1_out | y2_out) ? 0 : image_bits[y2_offset + x1];
        ARGB br = (x2_out | y2_out) ? 0 : image_bits[y2_offset + x2];

        ARGB xtop((tl.a * idistx + tr.a * distx) >> 8,
                  (tl.r * idistx + tr.r * distx) >> 8,
                  (tl.g * idistx + tr.g * distx) >> 8,
                  (tl.b * idistx + tr.b * distx) >> 8);


        ARGB xbot((bl.a * idistx + br.a * distx) >> 8,
                  (bl.r * idistx + br.r * distx) >> 8,
                  (bl.g * idistx + br.g * distx) >> 8,
                  (bl.b * idistx + br.b * distx) >> 8);


        ARGB res((xtop.a * idisty + xbot.a * disty) >> 8,
                 (xtop.r * idisty + xbot.r * disty) >> 8,
                 (xtop.g * idisty + xbot.g * disty) >> 8,
                 (xtop.b * idisty + xbot.b * disty) >> 8);

        qt_blend_pixel(res, target, span->coverage);
        x += fdx;
        y += fdy;
        ++target;
    }
}

static void blend_transformed_bilinear_tiled(ARGB *target,
                                              const QSpan *span,
                                              qreal ix, qreal iy, qreal dx, qreal dy,
                                              ARGB *image_bits, int image_width, int image_height)
{
    const int fixed_scale = 1 << 16;
    int x = int((ix + dx * span->x) * fixed_scale);
    int y = int((iy + dy * span->x) * fixed_scale);

    int fdx = (int)(dx * fixed_scale);
    int fdy = (int)(dy * fixed_scale);

    for (int i = 0; i < span->len; ++i) {
        int x1 = (x >> 16);
        int x2 = (x1 + 1);
        int y1 = (y >> 16);
        int y2 = (y1 + 1);

        int distx = ((x - (x1 << 16)) >> 8);
        int disty = ((y - (y1 << 16)) >> 8);
        int idistx = 256 - distx;
        int idisty = 256 - disty;

        x1 %= image_width;
        x2 %= image_width;
        y1 %= image_height;
        y2 %= image_height;

        if (x1 < 0) x1 = image_width + x1;
        if (x2 < 0) x2 = image_width + x2;
        if (y1 < 0) y1 = image_height + y1;
        if (y2 < 0) y2 = image_height + y2;

        Q_ASSERT(x1 >= 0 && x1 < image_width);
        Q_ASSERT(x2 >= 0 && x2 < image_width);
        Q_ASSERT(y1 >= 0 && y1 < image_height);
        Q_ASSERT(y2 >= 0 && y2 < image_height);

        int y1_offset = y1 * image_width;
        int y2_offset = y2 * image_width;

        ARGB tl = image_bits[y1_offset + x1];
        ARGB tr = image_bits[y1_offset + x2];
        ARGB bl = image_bits[y2_offset + x1];
        ARGB br = image_bits[y2_offset + x2];

        ARGB xtop((tl.a * idistx + tr.a * distx) >> 8,
                  (tl.r * idistx + tr.r * distx) >> 8,
                  (tl.g * idistx + tr.g * distx) >> 8,
                  (tl.b * idistx + tr.b * distx) >> 8);


        ARGB xbot((bl.a * idistx + br.a * distx) >> 8,
                  (bl.r * idistx + br.r * distx) >> 8,
                  (bl.g * idistx + br.g * distx) >> 8,
                  (bl.b * idistx + br.b * distx) >> 8);


        ARGB res((xtop.a * idisty + xbot.a * disty) >> 8,
                 (xtop.r * idisty + xbot.r * disty) >> 8,
                 (xtop.g * idisty + xbot.g * disty) >> 8,
                 (xtop.b * idisty + xbot.b * disty) >> 8);

        qt_blend_pixel(res, target, span->coverage);
        x += fdx;
        y += fdy;
        ++target;
    }
}


DrawHelper qDrawHelper =
{
    blend_color,
    blend_transformed_bilinear,
    blend_transformed_bilinear_tiled
};
