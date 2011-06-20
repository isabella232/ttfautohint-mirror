/* tafpgm.c */

/* written 2011 by Werner Lemberg <wl@gnu.org> */

#include "tabytecode.h"


/* we need the following macro */
/* so that `func_name' doesn't get replaced with its #defined value */
/* (as defined in `tabytecode.h') */

#define FPGM(func_name) fpgm_ ## func_name


/* in the comments below, the top of the stack (`s:') */
/* is the rightmost element; the stack is shown */
/* after the instruction on the same line has been executed */

/* we use two sets of points in the twilight zone (zp0): */
/* one set to hold the unhinted segment positions, */
/* and another one to track the positions as changed by the hinting -- */
/* this is necessary since all points in zp0 */
/* have (0,0) as the original coordinates, */
/* making e.g. `MD_orig' return useless results */


/*
 * bci_round
 *
 *   Round a 26.6 number.  Contrary to the ROUND bytecode instruction, no
 *   engine specific corrections are applied.
 *
 * in: val
 * out: ROUND(val)
 */

unsigned char FPGM(bci_round) [] = {

  PUSHB_1,
    bci_round,
  FDEF,

  DUP,
  ABS,
  PUSHB_1,
    32,
  ADD,
  FLOOR,
  SWAP,
  PUSHB_1,
    0,
  LT,
  IF,
    NEG,
  EIF,

  ENDF,

};


/*
 * bci_compute_stem_width
 *
 *   This is the equivalent to the following code from function
 *   `ta_latin_compute_stem_width':
 *
 *      dist = ABS(width)
 *
 *      if (stem_is_serif
 *          && dist < 3*64)
 *         || is_extra_light:
 *        return width
 *      else if base_is_round:
 *        if dist < 80
 *          dist = 64
 *      else if dist < 56:
 *        dist = 56
 *
 *      delta = ABS(dist - std_width)
 *
 *      if delta < 40:
 *        dist = std_width
 *        if dist < 48
 *          dist = 48
 *        goto End
 *
 *      if dist < 3*64:
 *        delta = dist
 *        dist = FLOOR(dist)
 *        delta = delta - dist
 *
 *        if delta < 10:
 *          dist = dist + delta
 *        else if delta < 32:
 *          dist = dist + 10
 *        else if delta < 54:
 *          dist = dist + 54
 *        else
 *          dist = dist + delta
 *      else
 *        dist = ROUND(dist)
 *
 *    End:
 *      if width < 0:
 *        dist = -dist
 *      return dist
 *
 *
 * in: width
 *     stem_is_serif
 *     base_is_round
 * out: new_width
 * sal: sal_is_extra_light
 * CVT: std_width
 */

unsigned char FPGM(bci_compute_stem_width_a) [] = {

  PUSHB_1,
    bci_compute_stem_width,
  FDEF,

  DUP,
  ABS, /* s: base_is_round stem_is_serif width dist */

  DUP,
  PUSHB_1,
    3*64,
  LT, /* dist < 3*64 */

  PUSHB_1,
    4,
  MINDEX, /* s: base_is_round width dist (dist<3*64) stem_is_serif */
  AND, /* stem_is_serif && dist < 3*64 */

  PUSHB_1,
    sal_is_extra_light,
  RS,
  OR, /* (stem_is_serif && dist < 3*64) || is_extra_light */

  IF, /* s: base_is_round width dist */
    POP,
    SWAP,
    POP, /* s: width */

  ELSE,
    ROLL, /* s: width dist base_is_round */
    IF, /* s: width dist */
      DUP,
      PUSHB_1,
        80,
      LT, /* dist < 80 */
      IF, /* s: width dist */
        POP,
        PUSHB_1,
          64, /* dist = 64 */
      EIF,

    ELSE,
      DUP,
      PUSHB_1,
        56,
      LT, /* dist < 56 */
      IF, /* s: width dist */
        POP,
        PUSHB_1,
          56, /* dist = 56 */
      EIF,
    EIF,

    DUP, /* s: width dist dist */
    PUSHB_1,

};

/*    %c, index of std_width */

unsigned char FPGM(bci_compute_stem_width_b) [] = {

    RCVT,
    SUB,
    ABS, /* s: width dist delta */

    PUSHB_1,
      40,
    LT, /* delta < 40 */
    IF, /* s: width dist */
      POP,
      PUSHB_1,

};

/*      %c, index of std_width */

unsigned char FPGM(bci_compute_stem_width_c) [] = {

      RCVT, /* dist = std_width */
      DUP,
      PUSHB_1,
        48,
      LT, /* dist < 48 */
      IF,
        POP,
        PUSHB_1,
          48, /* dist = 48 */
      EIF,

    ELSE,
      DUP, /* s: width dist dist */
      PUSHB_1,
        3*64,
      LT, /* dist < 3*64 */
      IF,
        DUP, /* s: width delta dist */
        FLOOR, /* dist = FLOOR(dist) */
        DUP, /* s: width delta dist dist */
        ROLL,
        ROLL, /* s: width dist delta dist */
        SUB, /* delta = delta - dist */

        DUP, /* s: width dist delta delta */
        PUSHB_1,
          10,
        LT, /* delta < 10 */
        IF, /* s: width dist delta */
          ADD, /* dist = dist + delta */

        ELSE,
          DUP,
          PUSHB_1,
            32,
          LT, /* delta < 32 */
          IF,
            POP,
            PUSHB_1,
              10,
            ADD, /* dist = dist + 10 */

          ELSE,
            DUP,
            PUSHB_1,
              54,
            LT, /* delta < 54 */
            IF,
              POP,
              PUSHB_1,
                54,
              ADD, /* dist = dist + 54 */

            ELSE,
              ADD, /* dist = dist + delta */

            EIF,
          EIF,
        EIF,

        ELSE,
          PUSHB_1,
            bci_round,
          CALL, /* dist = round(dist) */

        EIF,
      EIF,

      SWAP, /* s: dist width */
      PUSHB_1,
        0,
      LT, /* width < 0 */
      IF,
        NEG, /* dist = -dist */
      EIF,
    EIF,
  EIF,

  ENDF,

};


/*
 * bci_loop
 *
 *   Take a range and a function number and apply the function to all
 *   elements of the range.
 *
 * in: func_num
 *     end
 *     start
 *
 * uses: sal_i (counter initialized with `start')
 *       sal_limit (`end')
 *       sal_func (`func_num')
 */

unsigned char FPGM(bci_loop) [] = {

  PUSHB_1,
    bci_loop,
  FDEF,

  PUSHB_1,
    sal_func,
  SWAP,
  WS, /* sal_func = func_num */
  PUSHB_1,
    sal_limit,
  SWAP,
  WS, /* sal_limit = end */
  PUSHB_1,
    sal_i,
  SWAP,
  WS, /* sal_i = start */

/* start_loop: */
  PUSHB_1,
    sal_i,
  RS,
  PUSHB_1,
    sal_limit,
  RS,
  LTEQ, /* start <= end */
  IF,
    PUSHB_1,
      sal_func,
    RS,
    CALL,
    PUSHB_3,
      sal_i,
      1,
      sal_i,
    RS,
    ADD, /* start = start + 1 */
    WS,

    PUSHB_1,
      22,
    NEG,
    JMPR, /* goto start_loop */
  EIF,

  ENDF,

};


/*
 * bci_cvt_rescale
 *
 *   Rescale CVT value by a given factor.
 *
 * uses: sal_i (CVT index)
 *       sal_scale (scale in 16.16 format)
 */

unsigned char FPGM(bci_cvt_rescale) [] = {

  PUSHB_1,
    bci_cvt_rescale,
  FDEF,

  PUSHB_1,
    sal_i,
  RS,
  DUP,
  RCVT,
  PUSHB_1,
    sal_scale,
  RS,
  MUL, /* CVT * scale * 2^10 */
  PUSHB_1,
    sal_0x10000,
  RS,
  DIV, /* CVT * scale */

  WCVTP,

  ENDF,

};


/*
 * bci_blue_round
 *
 *   Round a blue ref value and adjust its corresponding shoot value.
 *
 * uses: sal_i (CVT index)
 *
 */

unsigned char FPGM(bci_blue_round_a) [] = {

  PUSHB_1,
    bci_blue_round,
  FDEF,

  PUSHB_1,
    sal_i,
  RS,
  DUP,
  RCVT, /* s: ref_idx ref */

  DUP,
  PUSHB_1,
    bci_round,
  CALL,
  SWAP, /* s: ref_idx round(ref) ref */

  PUSHB_2,

};

/*  %c, blue_count */

unsigned char FPGM(bci_blue_round_b) [] = {

    4,
  CINDEX,
  ADD, /* s: ref_idx round(ref) ref shoot_idx */
  DUP,
  RCVT, /* s: ref_idx round(ref) ref shoot_idx shoot */

  ROLL, /* s: ref_idx round(ref) shoot_idx shoot ref */
  SWAP,
  SUB, /* s: ref_idx round(ref) shoot_idx dist */
  DUP,
  ABS, /* s: ref_idx round(ref) shoot_idx dist delta */

  DUP,
  PUSHB_1,
    32,
  LT, /* delta < 32 */
  IF,
    POP,
    PUSHB_1,
      0, /* delta = 0 */

  ELSE,
    PUSHB_1,
      48,
    LT, /* delta < 48 */
    IF,
      PUSHB_1,
        32, /* delta = 32 */

    ELSE,
      PUSHB_1,
        64, /* delta = 64 */
    EIF,
  EIF,

  SWAP, /* s: ref_idx round(ref) shoot_idx delta dist */
  PUSHB_1,
    0,
  LT, /* dist < 0 */
  IF,
    NEG, /* delta = -delta */
  EIF,

  PUSHB_1,
    3,
  CINDEX,
  SWAP,
  SUB, /* s: ref_idx round(ref) shoot_idx (round(ref) - delta) */

  WCVTP,
  WCVTP,

  ENDF,

};


/*
 * bci_get_point_extrema
 *
 *   An auxiliary function for `bci_create_segment'.
 *
 * in: point-1
 * out: point
 *
 * sal: sal_point_min
 *      sal_point_max
 */

unsigned char FPGM(bci_get_point_extrema) [] = {

  PUSHB_1,
    bci_get_point_extrema,
  FDEF,

  PUSHB_1,
    1,
  ADD, /* s: point */
  DUP,
  DUP,

  /* check whether `point' is a new minimum */
  PUSHB_1,
    sal_point_min,
  RS, /* s: point point point point_min */
  MD_orig,
  /* if distance is negative, we have a new minimum */
  PUSHB_1,
    0,
  LT,
  IF, /* s: point point */
    DUP,
    PUSHB_1,
      sal_point_min,
    SWAP,
    WS,
  EIF,

  /* check whether `point' is a new maximum */
  PUSHB_1,
    sal_point_max,
  RS, /* s: point point point_max */
  MD_orig,
  /* if distance is positive, we have a new maximum */
  PUSHB_1,
    0,
  GT,
  IF, /* s: point */
    DUP,
    PUSHB_1,
      sal_point_max,
    SWAP,
    WS,
  EIF, /* s: point */

  ENDF,

};


/*
 * bci_create_segment
 *
 *   Store start and end point of a segment in the storage area,
 *   then construct two points in the twilight zone to represent it:
 *   an original one (which stays unmodified) and a hinted one,
 *   initialized with the original value.
 *
 *   This function is used by `bci_create_segment_points'.
 *
 * in: start
 *     end
 *       [last (if wrap-around segment)]
 *       [first (if wrap-around segment)]
 *
 * uses: bci_get_point_extrema
 *
 * sal: sal_i (start of current segment)
 *      sal_j (current original twilight point)
 *      sal_k (current hinted twilight point)
 *      sal_point_min
 *      sal_point_max
 *      sal_scale
 */

unsigned char FPGM(bci_create_segment) [] = {

  PUSHB_1,
    bci_create_segment,
  FDEF,

  PUSHB_1,
    sal_i,
  RS,
  PUSHB_1,
    2,
  CINDEX,
  WS, /* sal[sal_i] = start */

  /* increase `sal_i'; together with the outer loop, this makes sal_i += 2 */
  PUSHB_3,
    sal_i,
    1,
    sal_i,
  RS,
  ADD, /* sal_i = sal_i + 1 */
  WS,

  /* initialize inner loop(s) */
  PUSHB_2,
    sal_point_min,
    2,
  CINDEX,
  WS, /* sal_point_min = start */
  PUSHB_2,
    sal_point_max,
    2,
  CINDEX,
  WS, /* sal_point_max = start */

  PUSHB_1,
    1,
  SZPS, /* set zp0, zp1, and zp2 to normal zone 1 */

  SWAP,
  DUP,
  PUSHB_1,
    3,
  CINDEX, /* s: start end end start */
  LT, /* start > end */
  IF,
    /* we have a wrap-around segment with two more arguments */
    /* to give the last and first point of the contour, respectively; */
    /* our job is to store a segment `start'-`last', */
    /* and to get extrema for the two segments */
    /* `start'-`last' and `first'-`end' */

    /* s: first last start end */
    PUSHB_1,
      sal_i,
    RS,
    PUSHB_1,
      4,
    CINDEX,
    WS, /* sal[sal_i] = last */

    ROLL,
    ROLL, /* s: first end last start */
    DUP,
    ROLL,
    SWAP, /* s: first end start last start */
    SUB, /* s: first end start loop_count */

    PUSHB_1,
      bci_get_point_extrema,
    LOOPCALL,
    /* clean up stack */
    POP,

    SWAP, /* s: end first */
    PUSHB_1,
      1,
    SUB,
    DUP,
    ROLL, /* s: (first - 1) (first - 1) end */
    SWAP,
    SUB, /* s: (first - 1) loop_count */

    PUSHB_1,
      bci_get_point_extrema,
    LOOPCALL,
    /* clean up stack */
    POP,

  ELSE, /* s: start end */
    PUSHB_1,
      sal_i,
    RS,
    PUSHB_1,
      2,
    CINDEX,
    WS, /* sal[sal_i] = end */

    PUSHB_1,
      2,
    CINDEX,
    SUB, /* s: start loop_count */

    PUSHB_1,
      bci_get_point_extrema,
    LOOPCALL,
    /* clean up stack */
    POP,
  EIF,

  /* the twilight point representing a segment */
  /* is in the middle between the minimum and maximum */
  PUSHB_1,
    sal_point_min,
  RS,
  GC_orig,
  PUSHB_1,
    sal_point_max,
  RS,
  GC_orig,
  ADD,
  PUSHB_1,
    2*64,
  DIV, /* s: middle_pos */

  /* now scale it */
  PUSHB_1,
    sal_scale,
  RS,
  MUL, /* middle_pos * scale * 2^10 */
  PUSHB_1,
    sal_0x10000,
  RS,
  DIV, /* middle_pos = middle_pos * scale */

  DUP,
  PUSHB_2,
    sal_j,
    0,
  SZP2, /* set zp2 to twilight zone 0 */
  RS,
  DUP,
  GC_cur, /* s: middle_pos middle_pos point[sal_j] point[sal_j]_pos */
  ROLL,
  SWAP,
  SUB,
  SHPIX, /* align `point[sal_j]' with middle point */

  PUSHB_1,
    sal_k,
  RS,
  DUP,
  GC_cur, /* s: middle_pos point[sal_k] point[sal_k]_pos */
  ROLL,
  SWAP,
  SUB,
  SHPIX, /* align `point[sal_k]' with middle point */

  PUSHB_6,
    sal_k,
    1,
    sal_k,
    sal_j,
    1,
    sal_j,
  RS,
  ADD, /* original_twilight_point = original_twilight_point + 1 */
  WS,
  RS,
  ADD, /* hinted_twilight_point = hinted_twilight_point + 1 */
  WS,

  ENDF,

};


/*
 * bci_create_segments
 *
 *   Set up segments by defining point ranges which defines them
 *   and computing twilight points to represent them.
 *
 * in: num_segments (N)
 *     segment_start_0
 *     segment_end_0
 *       [contour_last 0 (if wrap-around segment)]
 *       [contour_first 0 (if wrap-around segment)]
 *     segment_start_1
 *     segment_end_1
 *       [contour_last 0 (if wrap-around segment)]
 *       [contour_first 0 (if wrap-around segment)]
 *     ...
 *     segment_start_(N-1)
 *     segment_end_(N-1)
 *       [contour_last (N-1) (if wrap-around segment)]
 *       [contour_first (N-1) (if wrap-around segment)]
 *
 * uses: bci_create_segment
 *
 * sal: sal_i (start of current segment)
 *      sal_j (current original twilight point)
 *      sal_k (current hinted twilight point)
 *      sal_num_segments
 *      sal_scale
 */

unsigned char FPGM(bci_create_segments) [] = {

  PUSHB_1,
    bci_create_segments,
  FDEF,

  /* all our measurements are taken along the y axis */
  SVTCA_y,

  PUSHB_1,
    sal_num_segments,
  SWAP,
  WS, /* sal_num_segments = num_segments */

  PUSHB_7,
    sal_segment_offset,
    sal_segment_offset,
    sal_num_segments,

    sal_k,
    0,
    sal_j,
    sal_num_segments,
  RS,
  WS, /* sal_j = num_segments (offset for original points) */
  WS, /* sal_k = 0 (offset for hinted points) */

  RS,
  DUP,
  ADD,
  ADD,
  PUSHB_1,
    1,
  SUB, /* s: sal_segment_offset (sal_segment_offset + 2*num_segments - 1) */

  /* `bci_create_segment_point' also increases the loop counter by 1; */
  /* this effectively means we have a loop step of 2 */
  PUSHB_2,
    bci_create_segment,
    bci_loop,
  CALL,

  ENDF,

};


/*
 * bci_align_segment
 *
 *   Align all points in a segment to the twilight point in rp0.
 *   zp0 and zp1 must be set to 0 (twilight) and 1 (normal), respectively.
 *
 * in: segment_index
 */

unsigned char FPGM(bci_align_segment) [] = {

  PUSHB_1,
    bci_align_segment,
  FDEF,

  /* we need the values of `sal_segment_offset + 2*segment_index' */
  /* and `sal_segment_offset + 2*segment_index + 1' */
  DUP,
  ADD,
  PUSHB_1,
    sal_segment_offset,
  ADD,
  DUP,
  RS,
  SWAP,
  PUSHB_1,
    1,
  ADD,
  RS, /* s: first last */

/* start_loop: */
  PUSHB_1,
    2,
  CINDEX, /* s: first last first */
  PUSHB_1,
    2,
  CINDEX, /* s: first last first last */
  LTEQ, /* first <= end */
  IF, /* s: first last */
    SWAP,
    DUP, /* s: last first first */
    ALIGNRP, /* align point with index `first' with rp0 */

    PUSHB_1,
      1,
    ADD, /* first = first + 1 */
    SWAP, /* s: first last */

    PUSHB_1,
      18,
    NEG,
    JMPR, /* goto start_loop */

  ELSE,
    POP,
    POP,
  EIF,

  ENDF,

};

/*
 * bci_align_segments
 *
 *   Align segments to the twilight point in rp0.
 *   zp0 and zp1 must be set to 0 (twilight) and 1 (normal), respectively.
 *
 * in: first_segment
 *     loop_counter (N)
 *       segment_1
 *       segment_2
 *       ...
 *       segment_N
 *
 * uses: handle_segment
 *
 */

unsigned char FPGM(bci_align_segments) [] = {

  PUSHB_1,
    bci_align_segments,
  FDEF,

  PUSHB_1,
    bci_align_segment,
  CALL,

  PUSHB_1,
    bci_align_segment,
  LOOPCALL,

  ENDF,

};


/*
 * bci_scale_contour
 *
 *   Scale a contour using two points giving the maximum and minimum
 *   coordinates.
 *
 *   It expects that all points on the contour are not touched.
 *
 * in: min_point
 *     max_point
 *
 * sal: sal_scale
 */

unsigned char FPGM(bci_scale_contour) [] = {

  PUSHB_1,
    bci_scale_contour,
  FDEF,

  DUP,
  DUP,
  GC_orig,
  DUP,
  PUSHB_1,
    sal_scale,
  RS,
  MUL, /* min_pos * scale * 2^10 */
  PUSHB_1,
    sal_0x10000,
  RS,
  DIV, /* min_pos_new = min_pos * scale */
  SWAP,
  SUB,
  SHPIX,

  /* don't scale a single-point contour twice */
  SWAP,
  DUP,
  ROLL,
  NEQ,
  IF,
    DUP,
    GC_orig,
    DUP,
    PUSHB_1,
      sal_scale,
    RS,
    MUL, /* max_pos * scale * 2^10 */
    PUSHB_1,
      sal_0x10000,
    RS,
    DIV, /* max_pos_new = max_pos * scale */
    SWAP,
    SUB,
    SHPIX,

  ELSE,
    POP,
  EIF,

  ENDF,

};


/*
 * bci_scale_glyph
 *
 *   Scale a glyph using a list of points (two points per contour, giving
 *   the maximum and mininum coordinates).
 *
 *   It expects that no point in the glyph is touched.
 *
 * in: num_contours (N)
 *       min_point_1
 *       max_point_1
 *       min_point_2
 *       max_point_2
 *       ...
 *       min_point_N
 *       max_point_N
 *
 * sal: sal_scale
 *
 * uses: bci_scale_contour
 */

unsigned char FPGM(bci_scale_glyph) [] = {

  PUSHB_1,
    bci_scale_glyph,
  FDEF,

  SVTCA_y,

  PUSHB_1,
    1,
  SZPS, /* set zp0, zp1, and zp2 to normal zone 1 */

  PUSHB_1,
    bci_scale_contour,
  LOOPCALL,

  IUP_y,

  ENDF,

};


/*
 * bci_shift_contour
 *
 *   Shift a contour by a given amount.
 *
 *   It expects that rp1 (point to by zp0) is set up properly; zp2 must
 *   point to the normal zone 1.
 *
 * in:  contour
 * out: contour + 1
 */

unsigned char FPGM(bci_shift_contour) [] = {

  PUSHB_1,
    bci_shift_contour,
  FDEF,

  DUP,
  SHC_rp1, /* shift `contour' by (rp1_pos - rp1_orig_pos) */

  PUSHB_1,
    1,
  ADD,

  ENDF,

};


/*
 * bci_shift_subglyph
 *
 *   Shift a subglyph.  To be more specific, it corrects the already applied
 *   subglyph offset (if any) from the `glyf' table which needs to be scaled
 *   also.
 *
 *   If this function is called, a point `x' in the subglyph has been scaled
 *   by `sal_scale' already (during the hinting of the subglyph itself), and
 *   `offset' has been applied also:
 *
 *     x  ->  x * scale + offset         (1)
 *
 *   However, the offset should be applied first, then the scaling:
 *
 *     x  ->  (x + offset) * scale       (2)
 *
 *   Our job is now to transform (1) to (2); a simple calculation shows that
 *   we have to shift all points of the subglyph by
 *
 *     offset * scale - offset = offset * (scale - 1)
 *
 * in: offset (in FUnits)
 *     num_contours
 *     first_contour
 *
 * sal: sal_scale
 */

unsigned char FPGM(bci_shift_subglyph) [] = {

  PUSHB_1,
    bci_shift_subglyph,
  FDEF,

  SVTCA_y,

  PUSHB_1,
    0,
  RCVT, /* scaling factor FUnits -> pixels */
  MUL,
  PUSHB_1,
    sal_0x10000,
  RS,
  DIV,

  /* the autohinter always rounds offsets */
  PUSHB_1,
    bci_round,
  CALL, /* offset = round(offset) */

  PUSHB_1,
    sal_scale,
  RS,
  PUSHB_1,
    sal_0x10000,
  RS,
  SUB, /* scale - 1 (in 16.16 format) */
  MUL,
  PUSHB_1,
    sal_0x10000,
  RS,
  DIV, /* delta = offset * (scale - 1) */

  /* and round again */
  PUSHB_1,
    bci_round,
  CALL, /* offset = round(offset) */

  PUSHB_1,
    0,
  SZPS, /* set zp0, zp1, and zp2 to normal zone 1 */

  /* we arbitrarily use twilight point 0 as the reference point; */
  PUSHB_2,
    0,
    0,
  MDAP_noround, /* set rp0 and rp1 to twilight point 0 */
  SWAP, /* c: first_contour num_contours 0 delta */

  PUSHB_1,
    0,
  GC_orig,
  PUSHB_1,
    0,
  GC_cur,
  SUB,
  ADD,
  SHPIX, /* rp1_pos - rp1_pos_orig = delta */

  PUSHB_2,
    bci_shift_contour,
    1,
  SZP2, /* set zp2 to normal zone 1 */
  LOOPCALL,

  ENDF,

};


/*
 * bci_ip_outer_align_point
 *
 *   Auxiliary function for `bci_action_ip_before' and
 *   `bci_action_ip_after'.
 *
 *   It expects rp0 to contain the edge for alignment, zp0 set to twilight
 *   zone, and both zp1 and zp2 set to normal zone.
 *
 * in: point
 *
 * sal: sal_i (edge_orig_pos)
 */

unsigned char FPGM(bci_ip_outer_align_point) [] = {

  PUSHB_1,
    bci_ip_outer_align_point,
  FDEF,

  DUP,
  ALIGNRP, /* align `point' with `edge' */
  DUP,
  GC_orig,
  /* now scale it */
  PUSHB_1,
    sal_scale,
  RS,
  MUL, /* point_orig_pos * scale * 2^10 */
  PUSHB_1,
    sal_0x10000,
  RS,
  DIV, /* point_orig_pos = point_orig_pos * scale */

  PUSHB_1,
    sal_i,
  RS,
  SUB, /* s: point (point_orig_pos - edge_orig_pos) */
  SHPIX,

  ENDF,

};


/*
 * bci_ip_on_align_points
 *
 *   Auxiliary function for `bci_action_ip_on'.
 *
 * in: edge
 *     loop_counter (N)
 *       point_1
 *       point_2
 *       ...
 *       point_N
 */

unsigned char FPGM(bci_ip_on_align_points) [] = {

  PUSHB_1,
    bci_ip_on_align_points,
  FDEF,

  MDAP_noround, /* set rp0 and rp1 to `edge' */

  SLOOP,
  ALIGNRP,

  ENDF,

};


/*
 * bci_ip_between_align_point
 *
 *   Auxiliary function for `bci_ip_between_align_points'.
 *
 *   It expects rp0 to contain the edge for alignment, zp0 set to twilight
 *   zone, and both zp1 and zp2 set to normal zone.
 *
 * in: point
 *
 * sal: sal_i (edge_orig_pos)
 *      sal_j (stretch_factor)
 */

unsigned char FPGM(bci_ip_between_align_point) [] = {

  PUSHB_1,
    bci_ip_between_align_point,
  FDEF,

  DUP,
  ALIGNRP, /* align `point' with `edge' */
  DUP,
  GC_orig,
  /* now scale it */
  PUSHB_1,
    sal_scale,
  RS,
  MUL, /* edge_orig_pos * scale * 2^10 */
  PUSHB_1,
    sal_0x10000,
  RS,
  DIV, /* edge_orig_pos = edge_orig_pos * scale */

  PUSHB_1,
    sal_i,
  RS,
  SUB, /* s: point (point_orig_pos - edge_orig_pos) */
  PUSHB_1,
    sal_j,
  RS,
  MUL, /* s: point delta */
  SHPIX,

  ENDF,

};


/*
 * bci_ip_between_align_points
 *
 *   Auxiliary function for `bci_action_ip_between'.
 *
 * in: after_edge
 *     before_edge
 *     loop_counter (N)
 *       point_1
 *       point_2
 *       ...
 *       point_N
 *
 * sal: sal_i (before_orig_pos)
 *      sal_j (stretch_factor)
 *
 * uses: bci_ip_between_align_point
 */

unsigned char FPGM(bci_ip_between_align_points) [] = {

  PUSHB_1,
    bci_ip_between_align_points,
  FDEF,

  PUSHB_2,
    2,
    0,
  SZPS, /* set zp0, zp1, and zp2 to twilight zone 0 */
  CINDEX,
  DUP,
  MDAP_noround, /* set rp0 and rp1 to `before' */
  PUSHB_1,
    sal_num_segments,
  RS,
  ADD, /* s: before after before_orig */
  DUP,
  GC_cur,
  PUSHB_1,
    sal_i,
  SWAP,
  WS, /* sal_i = before_orig_pos */
  PUSHB_1,
    2,
  CINDEX,
  PUSHB_1,
    sal_num_segments,
  RS,
  ADD, /* s: before after before_orig after_orig */

  MD_cur, /* a = after_orig_pos - before_orig_pos */
  ROLL,
  ROLL,
  MD_cur, /* b = after_pos - before_pos */
  SWAP,
  DIV, /* s: a/b */
  PUSHB_1,
    sal_j,
  SWAP,
  WS, /* sal_j = stretch_factor */

  PUSHB_3,
    bci_ip_between_align_point,
    1,
    1,
  SZP2, /* set zp2 to normal zone 1 */
  SZP1, /* set zp1 to normal zone 1 */
  LOOPCALL,

  ENDF,

};


/*
 * bci_action_ip_before
 *
 *   Handle `ip_before' data to align points located before the first edge.
 *
 * in: first_edge
 *     loop_counter (N)
 *       point_1
 *       point_2
 *       ...
 *       point_N
 *
 * sal: sal_i (first_edge_orig_pos)
 *
 * uses: bci_ip_outer_align_point
 */

unsigned char FPGM(bci_action_ip_before) [] = {

  PUSHB_1,
    bci_action_ip_before,
  FDEF,

  PUSHB_1,
    0,
  SZP2, /* set zp2 to normal zone 0 */

  DUP,
  PUSHB_1,
    sal_num_segments,
  RS,
  ADD,
  GC_cur,
  PUSHB_1,
    sal_i,
  SWAP,
  WS, /* sal_i = first_edge_orig_pos */

  PUSHB_3,
    0,
    1,
    1,
  SZP2, /* set zp2 to normal zone 1 */
  SZP1, /* set zp1 to normal zone 1 */
  SZP0, /* set zp0 to twilight zone 0 */

  MDAP_noround, /* set rp0 and rp1 to `first_edge' */

  PUSHB_1,
    bci_ip_outer_align_point,
  LOOPCALL,

  ENDF,

};


/*
 * bci_action_ip_after
 *
 *   Handle `ip_after' data to align points located after the last edge.
 *
 * in: last_edge
 *     loop_counter (N)
 *       point_1
 *       point_2
 *       ...
 *       point_N
 *
 * sal: sal_i (last_edge_orig_pos)
 *
 * uses: bci_ip_outer_align_point
 */

unsigned char FPGM(bci_action_ip_after) [] = {

  PUSHB_1,
    bci_action_ip_after,
  FDEF,

  PUSHB_1,
    0,
  SZP2, /* set zp2 to normal zone 0 */

  DUP,
  PUSHB_1,
    sal_num_segments,
  RS,
  ADD,
  GC_cur,
  PUSHB_1,
    sal_i,
  SWAP,
  WS, /* sal_i = last_edge_orig_pos */

  PUSHB_3,
    0,
    1,
    1,
  SZP2, /* set zp2 to normal zone 1 */
  SZP1, /* set zp1 to normal zone 1 */
  SZP0, /* set zp0 to twilight zone 0 */

  MDAP_noround, /* set rp0 and rp1 to `last_edge' */

  PUSHB_1,
    bci_ip_outer_align_point,
  LOOPCALL,

  ENDF,

};


/*
 * bci_action_ip_on
 *
 *   Handle `ip_on' data to align points located on an edge coordinate (but
 *   not part of an edge).
 *
 * in: loop_counter (M)
 *       edge_1
 *       loop_counter (N_1)
 *         point_1
 *         point_2
 *         ...
 *         point_N_1
 *       edge_2
 *       loop_counter (N_2)
 *         point_1
 *         point_2
 *         ...
 *         point_N_2
 *       ...
 *       edge_M
 *       loop_counter (N_M)
 *         point_1
 *         point_2
 *         ...
 *         point_N_M
 *
 * uses: bci_ip_on_align_points
 */

unsigned char FPGM(bci_action_ip_on) [] = {

  PUSHB_1,
    bci_action_ip_on,
  FDEF,

  PUSHB_2,
    0,
    1,
  SZP1, /* set zp1 to normal zone 1 */
  SZP0, /* set zp0 to twilight zone 0 */

  PUSHB_1,
    bci_ip_on_align_points,
  LOOPCALL,

  ENDF,

};


/*
 * bci_action_ip_between
 *
 *   Handle `ip_between' data to align points located between two edges.
 *
 * in: loop_counter (M)
 *       before_edge_1
 *       after_edge_1
 *       loop_counter (N_1)
 *         point_1
 *         point_2
 *         ...
 *         point_N_1
 *       before_edge_2
 *       after_edge_2
 *       loop_counter (N_2)
 *         point_1
 *         point_2
 *         ...
 *         point_N_2
 *       ...
 *       before_edge_M
 *       after_edge_M
 *       loop_counter (N_M)
 *         point_1
 *         point_2
 *         ...
 *         point_N_M
 *
 * uses: bci_ip_between_align_points
 */

unsigned char FPGM(bci_action_ip_between) [] = {

  PUSHB_1,
    bci_action_ip_between,
  FDEF,

  PUSHB_1,
    bci_ip_between_align_points,
  LOOPCALL,

  ENDF,

};


/*
 * bci_action_adjust_bound
 *
 *   Handle the ADJUST_BOUND action to align an edge of a stem if the other
 *   edge of the stem has already been moved, then moving it again if
 *   necessary to stay bound.
 *
 * in: edge2_is_serif
 *     edge_is_round
 *     edge_point (in twilight zone)
 *     edge2_point (in twilight zone)
 *     edge[-1] (in twilight zone)
 *     ... stuff for bci_align_segments (edge) ...
 */

unsigned char FPGM(bci_action_adjust_bound) [] = {

  PUSHB_1,
    bci_action_adjust_bound,
  FDEF,

  PUSHB_1,
    0,
  SZPS, /* set zp0, zp1, and zp2 to twilight zone 0 */

  PUSHB_1,
    4,
  CINDEX,
  PUSHB_1,
    sal_num_segments,
  RS,
  ADD, /* s: edge[-1] edge2 edge is_round is_serif edge2_orig */
  PUSHB_1,
    4,
  CINDEX,
  PUSHB_1,
    sal_num_segments,
  RS,
  ADD, /* s: edge[-1] edge2 edge is_round is_serif edge2_orig edge_orig */
  MD_cur, /* s: edge[-1] edge2 edge is_round is_serif org_len */

  PUSHB_1,
    bci_compute_stem_width,
  CALL,
  NEG, /* s: edge[-1] edge2 edge -cur_len */

  ROLL, /* s: edge[-1] edge -cur_len edge2 */
  MDAP_noround, /* set rp0 and rp1 to `edge2' */
  SWAP,
  DUP,
  DUP, /* s: edge[-1] -cur_len edge edge edge */
  ALIGNRP, /* align `edge' with `edge2' */
  ROLL,
  SHPIX, /* shift `edge' by -cur_len */

  SWAP, /* s: edge edge[-1] */
  DUP,
  MDAP_noround, /* set rp0 and rp1 to `edge[-1]' */
  GC_cur,
  PUSHB_1,
    2,
  CINDEX,
  GC_cur, /* s: edge edge[-1]_pos edge_pos */
  GT, /* edge_pos < edge[-1]_pos */
  IF,
    DUP,
    ALIGNRP, /* align `edge' to `edge[-1]' */
  EIF,

  MDAP_noround, /* set rp0 and rp1 to `edge' */

  PUSHB_2,
    bci_align_segments,
    1,
  SZP1, /* set zp1 to normal zone 1 */
  CALL,

  ENDF,

};


/*
 * bci_action_stem_bound
 *
 *   Handle the STEM action to align two edges of a stem, then moving one
 *   edge again if necessary to stay bound.
 *
 *   The code after computing `cur_len' to shift `edge' and `edge2'
 *   is equivalent to the snippet below (part of `ta_latin_hint_edges'):
 *
 *      if cur_len < 96:
 *        if cur_len < = 64:
 *          u_off = 32
 *          d_off = 32
 *        else:
 *          u_off = 38
 *          d_off = 26
 *
 *        org_pos = anchor + (edge_orig - anchor_orig);
 *        org_center = org_pos + org_len / 2;
 *
 *        cur_pos1 = ROUND(org_center)
 *        delta1 = ABS(org_center - (cur_pos1 - u_off))
 *        delta2 = ABS(org_center - (cur_pos1 + d_off))
 *        if (delta1 < delta2):
 *          cur_pos1 = cur_pos1 - u_off
 *        else:
 *          cur_pos1 = cur_pos1 + d_off
 *
 *        edge = cur_pos1 - cur_len / 2
 *
 *      else:
 *        org_pos = anchor + (edge_orig - anchor_orig)
 *        org_center = org_pos + org_len / 2;
 *
 *        cur_pos1 = ROUND(org_pos)
 *        delta1 = ABS(cur_pos1 + cur_len / 2 - org_center)
 *        cur_pos2 = ROUND(org_pos + org_len) - cur_len
 *        delta2 = ABS(cur_pos2 + cur_len / 2 - org_center)
 *
 *        if (delta1 < delta2):
 *          edge = cur_pos1
 *        else:
 *          edge = cur_pos2
 *
 *      edge2 = edge + cur_len
 *
 * in: edge2_is_serif
 *     edge_is_round
 *     edge_point (in twilight zone)
 *     edge2_point (in twilight zone)
 *     edge[-1] (in twilight zone)
 *     ... stuff for bci_align_segments (edge) ...
 *     ... stuff for bci_align_segments (edge2)...
 *
 * sal: sal_anchor
 *      sal_temp1
 *      sal_temp2
 *      sal_temp3
 *      sal_num_segments
 */

#undef sal_u_off
#define sal_u_off sal_temp1
#undef sal_d_off
#define sal_d_off sal_temp2
#undef sal_org_len
#define sal_org_len sal_temp3
#undef sal_edge2
#define sal_edge2 sal_temp3

unsigned char FPGM(bci_action_stem_bound) [] = {

  PUSHB_1,
    bci_action_stem_bound,
  FDEF,

  PUSHB_1,
    0,
  SZPS, /* set zp0, zp1, and zp2 to twilight zone 0 */

  PUSHB_1,
    4,
  CINDEX,
  PUSHB_1,
    sal_num_segments,
  RS,
  ADD,
  PUSHB_1,
    4,
  CINDEX,
  DUP,
  MDAP_noround, /* set rp0 and rp1 to `edge_point' (for ALIGNRP below) */
  PUSHB_1,
    sal_num_segments,
  RS,
  ADD, /* s: edge[-1] edge2 edge is_round is_serif edge2_orig edge_orig */

  MD_cur, /* s: edge[-1] edge2 edge is_round is_serif org_len */
  DUP,
  PUSHB_1,
    sal_org_len,
  SWAP,
  WS,

  PUSHB_1,
    bci_compute_stem_width,
  CALL, /* s: edge[-1] edge2 edge cur_len */

  DUP,
  PUSHB_1,
    96,
  LT, /* cur_len < 96 */
  IF,
    DUP,
    PUSHB_1,
      64,
    LTEQ, /* cur_len <= 64 */
    IF,
      PUSHB_4,
        sal_u_off,
        32,
        sal_d_off,
        32,

    ELSE,
      PUSHB_4,
        sal_u_off,
        38,
        sal_d_off,
        26,
    EIF,
    WS,
    WS,

    SWAP, /* s: edge[-1] edge2 cur_len edge */
    DUP,
    PUSHB_1,
      sal_num_segments,
    RS,
    ADD, /* s: edge[-1] edge2 cur_len edge edge_orig */
    PUSHB_1,
      sal_anchor,
    RS,
    DUP,
    PUSHB_1,
      sal_num_segments,
    RS,
    ADD, /* s: edge[-1] edge2 cur_len edge edge_orig anchor anchor_orig */
    ROLL,
    SWAP,
    MD_cur,
    SWAP,
    GC_cur,
    ADD, /* s: edge[-1] edge2 cur_len edge org_pos */
    PUSHB_1,
      sal_org_len,
    RS,
    PUSHB_1,
      2*64,
    DIV,
    ADD, /* s: edge[-1] edge2 cur_len edge org_center */

    DUP,
    PUSHB_1,
      bci_round,
    CALL, /* s: edge[-1] edge2 cur_len edge org_center cur_pos1 */

    DUP,
    ROLL,
    ROLL,
    SUB, /* s: ... cur_len edge cur_pos1 (org_center - cur_pos1) */

    DUP,
    PUSHB_1,
      sal_u_off,
    RS,
    ADD,
    ABS, /* s: ... cur_len edge cur_pos1 (org_center - cur_pos1) delta1 */

    SWAP,
    PUSHB_1,
      sal_d_off,
    RS,
    SUB,
    ABS, /* s: edge[-1] edge2 cur_len edge cur_pos1 delta1 delta2 */

    LT, /* delta1 < delta2 */
    IF,
      PUSHB_1,
        sal_u_off,
      RS,
      SUB, /* cur_pos1 = cur_pos1 - u_off */

    ELSE,
      PUSHB_1,
        sal_d_off,
      RS,
      ADD, /* cur_pos1 = cur_pos1 + d_off */
    EIF, /* s: edge[-1] edge2 cur_len edge cur_pos1 */

    PUSHB_1,
      3,
    CINDEX,
    PUSHB_1,
      2*64,
    DIV,
    SUB, /* arg = cur_pos1 - cur_len/2 */

    SWAP, /* s: edge[-1] edge2 cur_len arg edge */
    DUP,
    DUP,
    PUSHB_1,
      4,
    MINDEX,
    SWAP, /* s: edge[-1] edge2 cur_len edge edge arg edge */
    GC_cur,
    SUB,
    SHPIX, /* edge = cur_pos1 - cur_len/2 */

  ELSE,
    SWAP, /* s: edge[-1] edge2 cur_len edge */
    PUSHB_1,
      sal_anchor,
    RS,
    GC_cur, /* s: edge[-1] edge2 cur_len edge anchor_pos */
    PUSHB_1,
      2,
    CINDEX,
    PUSHB_1,
      sal_num_segments,
    RS,
    ADD,
    PUSHB_1,
      sal_anchor,
    RS,
    PUSHB_1,
      sal_num_segments,
    RS,
    ADD,
    MD_cur,
    ADD, /* s: edge[-1] edge2 cur_len edge org_pos */

    DUP,
    PUSHB_1,
      sal_org_len,
    RS,
    PUSHB_1,
      2*64,
    DIV,
    ADD, /* s: edge[-1] edge2 cur_len edge org_pos org_center */

    SWAP,
    DUP,
    PUSHB_1,
      bci_round,
    CALL, /* cur_pos1 = ROUND(org_pos) */
    SWAP,
    PUSHB_1,
      sal_org_len,
    RS,
    ADD,
    PUSHB_1,
      bci_round,
    CALL,
    PUSHB_1,
      5,
    CINDEX,
    SUB, /* s: edge[-1] edge2 cur_len edge org_center cur_pos1 cur_pos2 */

    PUSHB_1,
      5,
    CINDEX,
    PUSHB_1,
      2*64,
    DIV,
    PUSHB_1,
      4,
    MINDEX,
    SUB, /* s: ... cur_len edge cur_pos1 cur_pos2 (cur_len/2 - org_center) */

    DUP,
    PUSHB_1,
      4,
    CINDEX,
    ADD,
    ABS, /* delta1 = ABS(cur_pos1 + cur_len / 2 - org_center) */
    SWAP,
    PUSHB_1,
      3,
    CINDEX,
    ADD,
    ABS, /* s: ... edge2 cur_len edge cur_pos1 cur_pos2 delta1 delta2 */
    LT, /* delta1 < delta2 */
    IF,
      POP, /* arg = cur_pos1 */
    ELSE,
      SWAP,
      POP, /* arg = cur_pos2 */
    EIF, /* s: edge[-1] edge2 cur_len edge arg */
    SWAP,
    DUP,
    DUP,
    PUSHB_1,
      4,
    MINDEX,
    SWAP, /* s: edge[-1] edge2 cur_len edge edge arg edge */
    GC_cur,
    SUB,
    SHPIX, /* edge = arg */
  EIF, /* s: edge[-1] edge2 cur_len edge */

  ROLL, /* s: edge[-1] cur_len edge edge2 */
  DUP,
  DUP,
  ALIGNRP, /* align `edge2' with rp0 (still `edge') */
  PUSHB_1,
    sal_edge2,
  SWAP,
  WS, /* s: edge[-1] cur_len edge edge2 */
  ROLL,
  SHPIX, /* edge2 = edge + cur_len */

  SWAP, /* s: edge edge[-1] */
  DUP,
  MDAP_noround, /* set rp0 and rp1 to `edge[-1]' */
  GC_cur,
  PUSHB_1,
    2,
  CINDEX,
  GC_cur, /* s: edge edge[-1]_pos edge_pos */
  GT, /* edge_pos < edge[-1]_pos */
  IF,
    DUP,
    ALIGNRP, /* align `edge' to `edge[-1]' */
  EIF,

  MDAP_noround, /* set rp0 and rp1 to `edge' */

  PUSHB_2,
    bci_align_segments,
    1,
  SZP1, /* set zp1 to normal zone 1 */
  CALL,

  PUSHB_1,
    sal_edge2,
  RS,
  MDAP_noround, /* set rp0 and rp1 to `edge2' */

  PUSHB_1,
    bci_align_segments,
  CALL,

  ENDF,

};


/*
 * bci_action_link
 *
 *   Handle the LINK action to link an edge to another one.
 *
 * in: stem_is_serif
 *     base_is_round
 *     base_point (in twilight zone)
 *     stem_point (in twilight zone)
 *     ... stuff for bci_align_segments (base) ...
 */

unsigned char FPGM(bci_action_link) [] = {

  PUSHB_1,
    bci_action_link,
  FDEF,

  PUSHB_1,
    0,
  SZPS, /* set zp0, zp1, and zp2 to twilight zone 0 */

  PUSHB_1,
    4,
  CINDEX,
  PUSHB_1,
    sal_num_segments,
  RS,
  ADD,
  PUSHB_1,
    4,
  MINDEX,
  DUP,
  MDAP_noround, /* set rp0 and rp1 to `base_point' (for ALIGNRP below) */
  PUSHB_1,
    sal_num_segments,
  RS,
  ADD, /* s: stem is_round is_serif stem_orig base_orig */

  MD_cur, /* s: stem is_round is_serif dist_orig */

  PUSHB_1,
    bci_compute_stem_width,
  CALL, /* s: stem new_dist */

  SWAP,
  DUP,
  ALIGNRP, /* align `stem_point' with `base_point' */
  DUP,
  MDAP_noround, /* set rp0 and rp1 to `stem_point' */
  SWAP,
  SHPIX, /* stem_point = base_point + new_dist */

  PUSHB_2,
    bci_align_segments,
    1,
  SZP1, /* set zp1 to normal zone 1 */
  CALL,

  ENDF,

};


/*
 * bci_action_anchor
 *
 *   Handle the ANCHOR action to align two edges
 *   and to set the edge anchor.
 *
 *   The code after computing `cur_len' to shift `edge' and `edge2'
 *   is equivalent to the snippet below (part of `ta_latin_hint_edges'):
 *
 *      if cur_len < 96:
 *        if cur_len < = 64:
 *          u_off = 32
 *          d_off = 32
 *        else:
 *          u_off = 38
 *          d_off = 26
 *
 *        org_center = edge_orig + org_len / 2
 *        cur_pos1 = ROUND(org_center)
 *
 *        error1 = ABS(org_center - (cur_pos1 - u_off))
 *        error2 = ABS(org_center - (cur_pos1 + d_off))
 *        if (error1 < error2):
 *          cur_pos1 = cur_pos1 - u_off
 *        else:
 *          cur_pos1 = cur_pos1 + d_off
 *
 *        edge = cur_pos1 - cur_len / 2
 *        edge2 = edge + cur_len
 *
 *      else:
 *        edge = ROUND(edge_orig)
 *
 * in: edge2_is_serif
 *     edge_is_round
 *     edge_point (in twilight zone)
 *     edge2_point (in twilight zone)
 *     ... stuff for bci_align_segments (edge) ...
 *
 * sal: sal_anchor
 *      sal_temp1
 *      sal_temp2
 *      sal_temp3
 */

#undef sal_u_off
#define sal_u_off sal_temp1
#undef sal_d_off
#define sal_d_off sal_temp2
#undef sal_org_len
#define sal_org_len sal_temp3

unsigned char FPGM(bci_action_anchor) [] = {

  PUSHB_1,
    bci_action_anchor,
  FDEF,

  /* store anchor point number in `sal_anchor' */
  PUSHB_2,
    sal_anchor,
    4,
  CINDEX,
  WS, /* sal_anchor = edge_point */

  PUSHB_1,
    0,
  SZPS, /* set zp0, zp1, and zp2 to twilight zone 0 */

  PUSHB_1,
    4,
  CINDEX,
  PUSHB_1,
    sal_num_segments,
  RS,
  ADD,
  PUSHB_1,
    4,
  CINDEX,
  DUP,
  MDAP_noround, /* set rp0 and rp1 to `edge_point' (for ALIGNRP below) */
  PUSHB_1,
    sal_num_segments,
  RS,
  ADD, /* s: edge2 edge is_round is_serif edge2_orig edge_orig */

  MD_cur, /* s: edge2 edge is_round is_serif org_len */
  DUP,
  PUSHB_1,
    sal_org_len,
  SWAP,
  WS,

  PUSHB_1,
    bci_compute_stem_width,
  CALL, /* s: edge2 edge cur_len */

  DUP,
  PUSHB_1,
    96,
  LT, /* cur_len < 96 */
  IF,
    DUP,
    PUSHB_1,
      64,
    LTEQ, /* cur_len <= 64 */
    IF,
      PUSHB_4,
        sal_u_off,
        32,
        sal_d_off,
        32,

    ELSE,
      PUSHB_4,
        sal_u_off,
        38,
        sal_d_off,
        26,
    EIF,
    WS,
    WS,

    SWAP, /* s: edge2 cur_len edge */
    DUP,
    PUSHB_1,
      sal_num_segments,
    RS,
    ADD, /* s: edge2 cur_len edge edge_orig */

    GC_cur,
    PUSHB_1,
      sal_org_len,
    RS,
    PUSHB_1,
      2*64,
    DIV,
    ADD, /* s: edge2 cur_len edge org_center */

    DUP,
    PUSHB_1,
      bci_round,
    CALL, /* s: edge2 cur_len edge org_center cur_pos1 */

    DUP,
    ROLL,
    ROLL,
    SUB, /* s: edge2 cur_len edge cur_pos1 (org_center - cur_pos1) */

    DUP,
    PUSHB_1,
      sal_u_off,
    RS,
    ADD,
    ABS, /* s: edge2 cur_len edge cur_pos1 (org_center - cur_pos1) error1 */

    SWAP,
    PUSHB_1,
      sal_d_off,
    RS,
    SUB,
    ABS, /* s: edge2 cur_len edge cur_pos1 error1 error2 */

    LT, /* error1 < error2 */
    IF,
      PUSHB_1,
        sal_u_off,
      RS,
      SUB, /* cur_pos1 = cur_pos1 - u_off */

    ELSE,
      PUSHB_1,
        sal_d_off,
      RS,
      ADD, /* cur_pos1 = cur_pos1 + d_off */
    EIF, /* s: edge2 cur_len edge cur_pos1 */

    PUSHB_1,
      3,
    CINDEX,
    PUSHB_1,
      2*64,
    DIV,
    SUB, /* s: edge2 cur_len edge (cur_pos1 - cur_len/2) */

    PUSHB_1,
      2,
    CINDEX, /* s: edge2 cur_len edge (cur_pos1 - cur_len/2) edge */
    GC_cur,
    SUB,
    SHPIX, /* edge = cur_pos1 - cur_len/2 */

    SWAP, /* s: cur_len edge2 */
    DUP,
    ALIGNRP, /* align `edge2' with rp0 (still `edge') */
    SWAP,
    SHPIX, /* edge2 = edge1 + cur_len */

  ELSE,
    POP, /* s: edge2 edge */
    DUP,
    PUSHB_1,
      sal_num_segments,
    RS,
    ADD, /* s: edge2 edge edge_orig */

    MDAP_noround, /* set rp0 and rp1 to `edge_orig' */
    DUP,
    ALIGNRP, /* align `edge' with `edge_orig' */
    MDAP_round, /* round `edge' */

    /* clean up stack */
    POP,
  EIF,

  PUSHB_2,
    bci_align_segments,
    1,
  SZP1, /* set zp1 to normal zone 1 */
  CALL,

  ENDF,

};


/*
 * bci_action_blue_anchor
 *
 *   Handle the BLUE_ANCHOR action to align an edge with a blue zone
 *   and to set the edge anchor.
 *
 * in: anchor_point (in twilight zone)
 *     blue_cvt_idx
 *     edge_point (in twilight zone)
 *     ... stuff for bci_align_segments (edge) ...
 *
 * sal: sal_anchor
 */

unsigned char FPGM(bci_action_blue_anchor) [] = {

  PUSHB_1,
    bci_action_blue_anchor,
  FDEF,

  /* store anchor point number in `sal_anchor' */
  PUSHB_1,
    sal_anchor,
  SWAP,
  WS,

  PUSHB_1,
    0,
  SZP0, /* set zp0 to twilight zone 0 */

  /* move `edge_point' to `blue_cvt_idx' position */
  MIAP_noround, /* this also sets rp0 */

  PUSHB_2,
    bci_align_segments,
    1,
  SZP1, /* set zp1 to normal zone 1 */
  CALL,

  ENDF,

};


/*
 * bci_action_adjust
 *
 *   Handle the ADJUST action to align an edge of a stem if the other edge
 *   of the stem has already been moved.
 *
 * in: edge2_is_serif
 *     edge_is_round
 *     edge_point (in twilight zone)
 *     edge2_point (in twilight zone)
 *     ... stuff for bci_align_segments (edge) ...
 */

unsigned char FPGM(bci_action_adjust) [] = {

  PUSHB_1,
    bci_action_adjust,
  FDEF,

  PUSHB_1,
    0,
  SZPS, /* set zp0, zp1, and zp2 to twilight zone 0 */

  PUSHB_1,
    4,
  CINDEX,
  PUSHB_1,
    sal_num_segments,
  RS,
  ADD, /* s: edge2 edge is_round is_serif edge2_orig */
  PUSHB_1,
    4,
  CINDEX,
  PUSHB_1,
    sal_num_segments,
  RS,
  ADD, /* s: edge2 edge is_round is_serif edge2_orig edge_orig */
  MD_cur, /* s: edge2 edge is_round is_serif org_len */

  PUSHB_1,
    bci_compute_stem_width,
  CALL,
  NEG, /* s: edge2 edge -cur_len */

  ROLL,
  MDAP_noround, /* set rp0 and rp1 to `edge2' */
  SWAP,
  DUP,
  DUP, /* s: -cur_len edge edge edge */
  ALIGNRP, /* align `edge' with `edge2' */
  ROLL,
  SHPIX, /* shift `edge' by -cur_len */

  MDAP_noround, /* set rp0 and rp1 to `edge' */

  PUSHB_2,
    bci_align_segments,
    1,
  SZP1, /* set zp1 to normal zone 1 */
  CALL,

  ENDF,

};


/*
 * bci_action_stem
 *
 *   Handle the STEM action to align two edges of a stem.
 *
 *   The code after computing `cur_len' to shift `edge' and `edge2'
 *   is equivalent to the snippet below (part of `ta_latin_hint_edges'):
 *
 *      if cur_len < 96:
 *        if cur_len < = 64:
 *          u_off = 32
 *          d_off = 32
 *        else:
 *          u_off = 38
 *          d_off = 26
 *
 *        org_pos = anchor + (edge_orig - anchor_orig);
 *        org_center = org_pos + org_len / 2;
  *
 *        cur_pos1 = ROUND(org_center)
 *        delta1 = ABS(org_center - (cur_pos1 - u_off))
 *        delta2 = ABS(org_center - (cur_pos1 + d_off))
 *        if (delta1 < delta2):
 *          cur_pos1 = cur_pos1 - u_off
 *        else:
 *          cur_pos1 = cur_pos1 + d_off
 *
 *        edge = cur_pos1 - cur_len / 2
 *
 *      else:
 *        org_pos = anchor + (edge_orig - anchor_orig)
 *        org_center = org_pos + org_len / 2;
 *
 *        cur_pos1 = ROUND(org_pos)
 *        delta1 = ABS(cur_pos1 + cur_len / 2 - org_center)
 *        cur_pos2 = ROUND(org_pos + org_len) - cur_len
 *        delta2 = ABS(cur_pos2 + cur_len / 2 - org_center)
 *
 *        if (delta1 < delta2):
 *          edge = cur_pos1
 *        else:
 *          edge = cur_pos2
 *
 *      edge2 = edge + cur_len
 *
 * in: edge2_is_serif
 *     edge_is_round
 *     edge_point (in twilight zone)
 *     edge2_point (in twilight zone)
 *     ... stuff for bci_align_segments (edge) ...
 *     ... stuff for bci_align_segments (edge2)...
 *
 * sal: sal_anchor
 *      sal_temp1
 *      sal_temp2
 *      sal_temp3
 *      sal_num_segments
 */

#undef sal_u_off
#define sal_u_off sal_temp1
#undef sal_d_off
#define sal_d_off sal_temp2
#undef sal_org_len
#define sal_org_len sal_temp3
#undef sal_edge2
#define sal_edge2 sal_temp3

unsigned char FPGM(bci_action_stem) [] = {

  PUSHB_1,
    bci_action_stem,
  FDEF,

  PUSHB_1,
    0,
  SZPS, /* set zp0, zp1, and zp2 to twilight zone 0 */

  PUSHB_1,
    4,
  CINDEX,
  PUSHB_1,
    sal_num_segments,
  RS,
  ADD,
  PUSHB_1,
    4,
  CINDEX,
  DUP,
  MDAP_noround, /* set rp0 and rp1 to `edge_point' (for ALIGNRP below) */
  PUSHB_1,
    sal_num_segments,
  RS,
  ADD, /* s: edge2 edge is_round is_serif edge2_orig edge_orig */

  MD_cur, /* s: edge2 edge is_round is_serif org_len */
  DUP,
  PUSHB_1,
    sal_org_len,
  SWAP,
  WS,

  PUSHB_1,
    bci_compute_stem_width,
  CALL, /* s: edge2 edge cur_len */

  DUP,
  PUSHB_1,
    96,
  LT, /* cur_len < 96 */
  IF,
    DUP,
    PUSHB_1,
      64,
    LTEQ, /* cur_len <= 64 */
    IF,
      PUSHB_4,
        sal_u_off,
        32,
        sal_d_off,
        32,

    ELSE,
      PUSHB_4,
        sal_u_off,
        38,
        sal_d_off,
        26,
    EIF,
    WS,
    WS,

    SWAP, /* s: edge2 cur_len edge */
    DUP,
    PUSHB_1,
      sal_num_segments,
    RS,
    ADD, /* s: edge2 cur_len edge edge_orig */
    PUSHB_1,
      sal_anchor,
    RS,
    DUP,
    PUSHB_1,
      sal_num_segments,
    RS,
    ADD, /* s: edge2 cur_len edge edge_orig anchor anchor_orig */
    ROLL,
    SWAP,
    MD_cur,
    SWAP,
    GC_cur,
    ADD, /* s: edge2 cur_len edge org_pos */
    PUSHB_1,
      sal_org_len,
    RS,
    PUSHB_1,
      2*64,
    DIV,
    ADD, /* s: edge2 cur_len edge org_center */

    DUP,
    PUSHB_1,
      bci_round,
    CALL, /* s: edge2 cur_len edge org_center cur_pos1 */

    DUP,
    ROLL,
    ROLL,
    SUB, /* s: edge2 cur_len edge cur_pos1 (org_center - cur_pos1) */

    DUP,
    PUSHB_1,
      sal_u_off,
    RS,
    ADD,
    ABS, /* s: ... cur_len edge cur_pos1 (org_center - cur_pos1) delta1 */

    SWAP,
    PUSHB_1,
      sal_d_off,
    RS,
    SUB,
    ABS, /* s: edge2 cur_len edge cur_pos1 delta1 delta2 */

    LT, /* delta1 < delta2 */
    IF,
      PUSHB_1,
        sal_u_off,
      RS,
      SUB, /* cur_pos1 = cur_pos1 - u_off */

    ELSE,
      PUSHB_1,
        sal_d_off,
      RS,
      ADD, /* cur_pos1 = cur_pos1 + d_off */
    EIF, /* s: edge2 cur_len edge cur_pos1 */

    PUSHB_1,
      3,
    CINDEX,
    PUSHB_1,
      2*64,
    DIV,
    SUB, /* arg = cur_pos1 - cur_len/2 */

    SWAP, /* s: edge2 cur_len arg edge */
    DUP,
    PUSHB_1,
      3,
    MINDEX,
    SWAP, /* s: edge2 cur_len edge arg edge */
    GC_cur,
    SUB,
    SHPIX, /* edge = cur_pos1 - cur_len/2 */

  ELSE,
    SWAP, /* s: edge2 cur_len edge */
    PUSHB_1,
      sal_anchor,
    RS,
    GC_cur, /* s: edge2 cur_len edge anchor_pos */
    PUSHB_1,
      2,
    CINDEX,
    PUSHB_1,
      sal_num_segments,
    RS,
    ADD,
    PUSHB_1,
      sal_anchor,
    RS,
    PUSHB_1,
      sal_num_segments,
    RS,
    ADD,
    MD_cur,
    ADD, /* s: edge2 cur_len edge org_pos */

    DUP,
    PUSHB_1,
      sal_org_len,
    RS,
    PUSHB_1,
      2*64,
    DIV,
    ADD, /* s: edge2 cur_len edge org_pos org_center */

    SWAP,
    DUP,
    PUSHB_1,
      bci_round,
    CALL, /* cur_pos1 = ROUND(org_pos) */
    SWAP,
    PUSHB_1,
      sal_org_len,
    RS,
    ADD,
    PUSHB_1,
      bci_round,
    CALL,
    PUSHB_1,
      5,
    CINDEX,
    SUB, /* s: edge2 cur_len edge org_center cur_pos1 cur_pos2 */

    PUSHB_1,
      5,
    CINDEX,
    PUSHB_1,
      2*64,
    DIV,
    PUSHB_1,
      4,
    MINDEX,
    SUB, /* s: ... cur_len edge cur_pos1 cur_pos2 (cur_len/2 - org_center) */

    DUP,
    PUSHB_1,
      4,
    CINDEX,
    ADD,
    ABS, /* delta1 = ABS(cur_pos1 + cur_len / 2 - org_center) */
    SWAP,
    PUSHB_1,
      3,
    CINDEX,
    ADD,
    ABS, /* s: edge2 cur_len edge cur_pos1 cur_pos2 delta1 delta2 */
    LT, /* delta1 < delta2 */
    IF,
      POP, /* arg = cur_pos1 */
    ELSE,
      SWAP,
      POP, /* arg = cur_pos2 */
    EIF, /* s: edge2 cur_len edge arg */
    SWAP,
    DUP,
    PUSHB_1,
      3,
    MINDEX,
    SWAP, /* s: edge2 cur_len edge arg edge */
    GC_cur,
    SUB,
    SHPIX, /* edge = arg */
  EIF, /* s: edge2 cur_len */

  SWAP, /* s: cur_len edge2 */
  DUP,
  DUP,
  ALIGNRP, /* align `edge2' with rp0 (still `edge') */
  PUSHB_1,
    sal_edge2,
  SWAP,
  WS, /* s: cur_len edge2 */
  SWAP,
  SHPIX, /* edge2 = edge + cur_len */

  PUSHB_2,
    bci_align_segments,
    1,
  SZP1, /* set zp1 to normal zone 1 */
  CALL,

  PUSHB_1,
    sal_edge2,
  RS,
  MDAP_noround, /* set rp0 and rp1 to `edge2' */

  PUSHB_1,
    bci_align_segments,
  CALL,
  ENDF,

};


/*
 * bci_action_blue
 *
 *   Handle the BLUE action to align an edge with a blue zone.
 *
 * in: blue_cvt_idx
 *     edge_point (in twilight zone)
 *     ... stuff for bci_align_segments (edge) ...
 */

unsigned char FPGM(bci_action_blue) [] = {

  PUSHB_1,
    bci_action_blue,
  FDEF,

  PUSHB_1,
    0,
  SZP0, /* set zp0 to twilight zone 0 */

  /* move `edge_point' to `blue_cvt_idx' position */
  MIAP_noround, /* this also sets rp0 */

  PUSHB_2,
    bci_align_segments,
    1,
  SZP1, /* set zp1 to normal zone 1 */
  CALL,

  ENDF,

};


/*
 * bci_action_serif
 *
 *   Handle the SERIF action to align a serif with its base.
 *
 * in: serif_point (in twilight zone)
 *     base_point (in twilight zone)
 *     ... stuff for bci_align_segments (serif) ...
 */

unsigned char FPGM(bci_action_serif) [] = {

  PUSHB_1,
    bci_action_serif,
  FDEF,

  PUSHB_1,
    0,
  SZPS, /* set zp0, zp1, and zp2 to twilight zone 0 */

  DUP,
  DUP,
  PUSHB_1,
    4,
  MINDEX, /* s: serif serif serif base */
  PUSHB_1,
    2,
  CINDEX,
  PUSHB_1,
    sal_num_segments,
  RS,
  ADD, /* s: serif serif serif base serif_orig */
  SWAP,
  DUP,
  MDAP_noround, /* set rp0 and rp1 to `base_point' */
  PUSHB_1,
    sal_num_segments,
  RS,
  ADD, /* s: serif serif serif serif_orig base_orig */
  MD_cur,
  SWAP,
  ALIGNRP, /* align `serif_point' with `base_point' */
  SHPIX, /* serif = base + (serif_orig_pos - base_orig_pos) */

  MDAP_noround, /* set rp0 and rp1 to `serif_point' */

  PUSHB_2,
    bci_align_segments,
    1,
  SZP1, /* set zp1 to normal zone 1 */
  CALL,

  ENDF,

};


/*
 * bci_action_serif_lower_bound
 *
 *   Handle the SERIF action to align a serif with its base, then moving it
 *   again if necessary to stay within a lower bound.
 *
 * in: serif_point (in twilight zone)
 *     base_point (in twilight zone)
 *     edge[-1] (in twilight zone)
 *     ... stuff for bci_align_segments (serif) ...
 */

unsigned char FPGM(bci_action_serif_lower_bound) [] = {

  PUSHB_1,
    bci_action_serif_lower_bound,
  FDEF,

  PUSHB_1,
    0,
  SZPS, /* set zp0, zp1, and zp2 to twilight zone 0 */

  DUP,
  DUP,
  PUSHB_1,
    4,
  MINDEX, /* s: edge[-1] serif serif serif base */
  PUSHB_1,
    2,
  CINDEX,
  PUSHB_1,
    sal_num_segments,
  RS,
  ADD, /* s: edge[-1] serif serif serif base serif_orig */
  SWAP,
  DUP,
  MDAP_noround, /* set rp0 and rp1 to `base_point' */
  PUSHB_1,
    sal_num_segments,
  RS,
  ADD, /* s: edge[-1] serif serif serif serif_orig base_orig */
  MD_cur,
  SWAP,
  ALIGNRP, /* align `serif_point' with `base_point' */
  SHPIX, /* serif = base + (serif_orig_pos - base_orig_pos) */

  SWAP, /* s: serif edge[-1] */
  DUP,
  MDAP_noround, /* set rp0 and rp1 to `edge[-1]' */
  GC_cur,
  PUSHB_1,
    2,
  CINDEX,
  GC_cur, /* s: serif edge[-1]_pos serif_pos */
  GT, /* serif_pos < edge[-1]_pos */
  IF,
    DUP,
    ALIGNRP, /* align `serif' to `edge[-1]' */
  EIF,

  MDAP_noround, /* set rp0 and rp1 to `serif_point' */

  PUSHB_2,
    bci_align_segments,
    1,
  SZP1, /* set zp1 to normal zone 1 */
  CALL,

  ENDF,

};


/*
 * bci_action_serif_upper_bound
 *
 *   Handle the SERIF action to align a serif with its base, then moving it
 *   again if necessary to stay within an upper bound.
 *
 * in: serif_point (in twilight zone)
 *     base_point (in twilight zone)
 *     edge[1] (in twilight zone)
 *     ... stuff for bci_align_segments (serif) ...
 */

unsigned char FPGM(bci_action_serif_upper_bound) [] = {

  PUSHB_1,
    bci_action_serif_upper_bound,
  FDEF,

  PUSHB_1,
    0,
  SZPS, /* set zp0, zp1, and zp2 to twilight zone 0 */

  DUP,
  DUP,
  PUSHB_1,
    4,
  MINDEX, /* s: edge[1] serif serif serif base */
  PUSHB_1,
    2,
  CINDEX,
  PUSHB_1,
    sal_num_segments,
  RS,
  ADD, /* s: edge[1] serif serif serif base serif_orig */
  SWAP,
  DUP,
  MDAP_noround, /* set rp0 and rp1 to `base_point' */
  PUSHB_1,
    sal_num_segments,
  RS,
  ADD, /* s: edge[1] serif serif serif serif_orig base_orig */
  MD_cur,
  SWAP,
  ALIGNRP, /* align `serif_point' with `base_point' */
  SHPIX, /* serif = base + (serif_orig_pos - base_orig_pos) */

  SWAP, /* s: serif edge[1] */
  DUP,
  MDAP_noround, /* set rp0 and rp1 to `edge[1]' */
  GC_cur,
  PUSHB_1,
    2,
  CINDEX,
  GC_cur, /* s: serif edge[1]_pos serif_pos */
  LT, /* serif_pos > edge[1]_pos */
  IF,
    DUP,
    ALIGNRP, /* align `serif' to `edge[1]' */
  EIF,

  MDAP_noround, /* set rp0 and rp1 to `serif_point' */

  PUSHB_2,
    bci_align_segments,
    1,
  SZP1, /* set zp1 to normal zone 1 */
  CALL,

  ENDF,

};


/*
 * bci_action_serif_lower_upper_bound
 *
 *   Handle the SERIF action to align a serif with its base, then moving it
 *   again if necessary to stay within a lower and upper bound.
 *
 * in: serif_point (in twilight zone)
 *     base_point (in twilight zone)
 *     edge[-1] (in twilight zone)
 *     edge[1] (in twilight zone)
 *     ... stuff for bci_align_segments (serif) ...
 */

unsigned char FPGM(bci_action_serif_lower_upper_bound) [] = {

  PUSHB_1,
    bci_action_serif_lower_upper_bound,
  FDEF,

  PUSHB_1,
    0,
  SZPS, /* set zp0, zp1, and zp2 to twilight zone 0 */

  DUP,
  DUP,
  PUSHB_1,
    4,
  MINDEX, /* s: edge[1] edge[-1] serif serif serif base */
  PUSHB_1,
    2,
  CINDEX,
  PUSHB_1,
    sal_num_segments,
  RS,
  ADD, /* s: edge[1] edge[-1] serif serif serif base serif_orig */
  SWAP,
  DUP,
  MDAP_noround, /* set rp0 and rp1 to `base_point' */
  PUSHB_1,
    sal_num_segments,
  RS,
  ADD, /* s: edge[1] edge[-1] serif serif serif serif_orig base_orig */
  MD_cur,
  SWAP,
  ALIGNRP, /* align `serif_point' with `base_point' */
  SHPIX, /* serif = base + (serif_orig_pos - base_orig_pos) */

  SWAP, /* s: edge[1] serif edge[-1] */
  DUP,
  MDAP_noround, /* set rp0 and rp1 to `edge[-1]' */
  GC_cur,
  PUSHB_1,
    2,
  CINDEX,
  GC_cur, /* s: edge[1] serif edge[-1]_pos serif_pos */
  GT, /* serif_pos < edge[-1]_pos */
  IF,
    DUP,
    ALIGNRP, /* align `serif' to `edge[-1]' */
  EIF,

  SWAP, /* s: serif edge[1] */
  DUP,
  MDAP_noround, /* set rp0 and rp1 to `edge[1]' */
  GC_cur,
  PUSHB_1,
    2,
  CINDEX,
  GC_cur, /* s: serif edge[1]_pos serif_pos */
  LT, /* serif_pos > edge[1]_pos */
  IF,
    DUP,
    ALIGNRP, /* align `serif' to `edge[1]' */
  EIF,

  MDAP_noround, /* set rp0 and rp1 to `serif_point' */

  PUSHB_2,
    bci_align_segments,
    1,
  SZP1, /* set zp1 to normal zone 1 */
  CALL,

  ENDF,

};


/*
 * bci_action_serif_anchor
 *
 *   Handle the SERIF_ANCHOR action to align a serif and to set the edge
 *   anchor.
 *
 * in: edge_point (in twilight zone)
 *     ... stuff for bci_align_segments (edge) ...
 */

unsigned char FPGM(bci_action_serif_anchor) [] = {

  PUSHB_1,
    bci_action_serif_anchor,
  FDEF,

  PUSHB_1,
    0,
  SZPS, /* set zp0, zp1, and zp2 to twilight zone 0 */

  DUP,
  PUSHB_1,
    sal_anchor,
  SWAP,
  WS, /* sal_anchor = edge_point */

  DUP,
  PUSHB_1,
    sal_num_segments,
  RS,
  ADD, /* s: edge edge_orig */

  MDAP_noround, /* set rp0 and rp1 to `edge_orig' */
  DUP,
  DUP,
  ALIGNRP, /* align `edge' with `edge_orig' */
  MDAP_round, /* round `edge' */

  MDAP_noround, /* set rp0 and rp1 to `edge' */

  PUSHB_2,
    bci_align_segments,
    1,
  SZP1, /* set zp1 to normal zone 1 */
  CALL,

  ENDF,

};


/*
 * bci_action_serif_anchor_lower_bound
 *
 *   Handle the SERIF_ANCHOR action to align a serif and to set the edge
 *   anchor, then moving it again if necessary to stay within a lower
 *   bound.
 *
 * in: edge_point (in twilight zone)
 *     edge[-1] (in twilight zone)
 *     ... stuff for bci_align_segments (edge) ...
 */

unsigned char FPGM(bci_action_serif_anchor_lower_bound) [] = {

  PUSHB_1,
    bci_action_serif_anchor_lower_bound,
  FDEF,

  PUSHB_1,
    0,
  SZPS, /* set zp0, zp1, and zp2 to twilight zone 0 */

  DUP,
  PUSHB_1,
    sal_anchor,
  SWAP,
  WS, /* sal_anchor = edge_point */

  DUP,
  PUSHB_1,
    sal_num_segments,
  RS,
  ADD, /* s: edge[-1] edge edge_orig */

  MDAP_noround, /* set rp0 and rp1 to `edge_orig' */
  DUP,
  DUP,
  ALIGNRP, /* align `edge' with `edge_orig' */
  MDAP_round, /* round `edge' */

  SWAP, /* s: edge edge[-1] */
  DUP,
  MDAP_noround, /* set rp0 and rp1 to `edge[-1]' */
  GC_cur,
  PUSHB_1,
    2,
  CINDEX,
  GC_cur, /* s: edge edge[-1]_pos edge_pos */
  GT, /* edge_pos < edge[-1]_pos */
  IF,
    DUP,
    ALIGNRP, /* align `edge' to `edge[-1]' */
  EIF,

  MDAP_noround, /* set rp0 and rp1 to `edge' */

  PUSHB_2,
    bci_align_segments,
    1,
  SZP1, /* set zp1 to normal zone 1 */
  CALL,

  ENDF,

};


/*
 * bci_action_serif_anchor_upper_bound
 *
 *   Handle the SERIF_ANCHOR action to align a serif and to set the edge
 *   anchor, then moving it again if necessary to stay within an upper
 *   bound.
 *
 * in: edge_point (in twilight zone)
 *     edge[1] (in twilight zone)
 *     ... stuff for bci_align_segments (edge) ...
 */

unsigned char FPGM(bci_action_serif_anchor_upper_bound) [] = {

  PUSHB_1,
    bci_action_serif_anchor_upper_bound,
  FDEF,

  PUSHB_1,
    0,
  SZPS, /* set zp0, zp1, and zp2 to twilight zone 0 */

  DUP,
  PUSHB_1,
    sal_anchor,
  SWAP,
  WS, /* sal_anchor = edge_point */

  DUP,
  PUSHB_1,
    sal_num_segments,
  RS,
  ADD, /* s: edge[1] edge edge_orig */

  MDAP_noround, /* set rp0 and rp1 to `edge_orig' */
  DUP,
  DUP,
  ALIGNRP, /* align `edge' with `edge_orig' */
  MDAP_round, /* round `edge' */

  SWAP, /* s: edge edge[1] */
  DUP,
  MDAP_noround, /* set rp0 and rp1 to `edge[1]' */
  GC_cur,
  PUSHB_1,
    2,
  CINDEX,
  GC_cur, /* s: edge edge[1]_pos edge_pos */
  LT, /* edge_pos > edge[1]_pos */
  IF,
    DUP,
    ALIGNRP, /* align `edge' to `edge[1]' */
  EIF,

  MDAP_noround, /* set rp0 and rp1 to `edge' */

  PUSHB_2,
    bci_align_segments,
    1,
  SZP1, /* set zp1 to normal zone 1 */
  CALL,

  ENDF,

};


/*
 * bci_action_serif_anchor_lower_upper_bound
 *
 *   Handle the SERIF_ANCHOR action to align a serif and to set the edge
 *   anchor, then moving it again if necessary to stay within a lower and
 *   upper bound.
 *
 * in: edge_point (in twilight zone)
 *     edge[-1] (in twilight zone)
 *     edge[1] (in twilight zone)
 *     ... stuff for bci_align_segments (edge) ...
 */

unsigned char FPGM(bci_action_serif_anchor_lower_upper_bound) [] = {

  PUSHB_1,
    bci_action_serif_anchor_lower_upper_bound,
  FDEF,

  PUSHB_1,
    0,
  SZPS, /* set zp0, zp1, and zp2 to twilight zone 0 */

  DUP,
  PUSHB_1,
    sal_anchor,
  SWAP,
  WS, /* sal_anchor = edge_point */

  DUP,
  PUSHB_1,
    sal_num_segments,
  RS,
  ADD, /* s: edge[1] edge[-1] edge edge_orig */

  MDAP_noround, /* set rp0 and rp1 to `edge_orig' */
  DUP,
  DUP,
  ALIGNRP, /* align `edge' with `edge_orig' */
  MDAP_round, /* round `edge' */

  SWAP, /* s: edge[1] edge edge[-1] */
  DUP,
  MDAP_noround, /* set rp0 and rp1 to `edge[-1]' */
  GC_cur,
  PUSHB_1,
    2,
  CINDEX,
  GC_cur, /* s: edge[1] edge edge[-1]_pos edge_pos */
  GT, /* edge_pos < edge[-1]_pos */
  IF,
    DUP,
    ALIGNRP, /* align `edge' to `edge[-1]' */
  EIF,

  SWAP, /* s: edge edge[1] */
  DUP,
  MDAP_noround, /* set rp0 and rp1 to `edge[1]' */
  GC_cur,
  PUSHB_1,
    2,
  CINDEX,
  GC_cur, /* s: edge edge[1]_pos edge_pos */
  LT, /* edge_pos > edge[1]_pos */
  IF,
    DUP,
    ALIGNRP, /* align `edge' to `edge[1]' */
  EIF,

  MDAP_noround, /* set rp0 and rp1 to `edge' */

  PUSHB_2,
    bci_align_segments,
    1,
  SZP1, /* set zp1 to normal zone 1 */
  CALL,

  ENDF,

};


/*
 * bci_action_serif_link1
 *
 *   Handle the SERIF_LINK1 action to align a serif, depending on edges
 *   before and after.
 *
 * in: before_point (in twilight zone)
 *     edge_point (in twilight zone)
 *     after_point (in twilight zone)
 *     ... stuff for bci_align_segments (edge) ...
 */

unsigned char FPGM(bci_action_serif_link1) [] = {

  PUSHB_1,
    bci_action_serif_link1,
  FDEF,

  PUSHB_1,
    0,
  SZPS, /* set zp0, zp1, and zp2 to twilight zone 0 */

  PUSHB_1,
    3,
  CINDEX,
  PUSHB_1,
    sal_num_segments,
  RS,
  ADD, /* s: after edge before after_orig */
  PUSHB_1,
    2,
  CINDEX,
  PUSHB_1,
    sal_num_segments,
  RS,
  ADD, /* s: after edge before after_orig before_orig */
  MD_cur,
  PUSHB_1,
    0,
  EQ, /* after_orig_pos == before_orig_pos */
  IF, /* s: after edge before */
    MDAP_noround, /* set rp0 and rp1 to `before' */
    DUP,
    ALIGNRP, /* align `edge' with `before' */
    SWAP,
    POP,

  ELSE,
    PUSHB_1,
      2,
    CINDEX,
    PUSHB_1,
      sal_num_segments,
    RS,
    ADD, /* s: ... after edge before edge_orig */
    PUSHB_1,
      2,
    CINDEX,
    PUSHB_1,
      sal_num_segments,
    RS,
    ADD, /* s: ... after edge before edge_orig before_orig */
    MD_cur, /* a = edge_orig_pos - before_orig_pos */
    PUSHW_1,
      0x10, /* 64*64 */
      0x00,
    MUL,

    PUSHB_1,
      4,
    CINDEX, /* s: ... after edge before a*64 after */
    PUSHB_1,
      3,
    CINDEX, /* s: ... after edge before a*64 after before */
    MD_cur, /* b = after_pos - before_pos */
    MUL, /* s: ... after edge before a*b */

    PUSHB_1,
      4,
    CINDEX,
    PUSHB_1,
      sal_num_segments,
    RS,
    ADD, /* s: ... after edge before a*b after_orig */
    PUSHB_1,
      3,
    CINDEX,
    PUSHB_1,
      sal_num_segments,
    RS,
    ADD, /* s: ... after edge before a*b after_orig before_orig */
    MD_cur, /* c = after_orig_pos - before_orig_pos */
    PUSHW_1,
      0x10, /* 64*64 */
      0x00,
    MUL,

    DIV, /* s: after edge before a*b/c */

    SWAP,
    MDAP_noround, /* set rp0 and rp1 to `before' */
    SWAP, /* s: after a*b/c edge */
    DUP,
    DUP,
    ALIGNRP, /* align `edge' with `before' */
    ROLL,
    SHPIX, /* shift `edge' by `a*b/c' */

    SWAP, /* s: edge after */
    POP,
  EIF,

  MDAP_noround, /* set rp0 and rp1 to `edge' */

  PUSHB_2,
    bci_align_segments,
    1,
  SZP1, /* set zp1 to normal zone 1 */
  CALL,

  ENDF,

};


/*
 * bci_action_serif_link1_lower_bound
 *
 *   Handle the SERIF_LINK1 action to align a serif, depending on edges
 *   before and after.  Additionally, move the serif again if necessary to
 *   stay within a lower bound.
 *
 * in: before_point (in twilight zone)
 *     edge_point (in twilight zone)
 *     after_point (in twilight zone)
 *     edge[-1] (in twilight zone)
 *     ... stuff for bci_align_segments (edge) ...
 */

unsigned char FPGM(bci_action_serif_link1_lower_bound) [] = {

  PUSHB_1,
    bci_action_serif_link1_lower_bound,
  FDEF,

  PUSHB_1,
    0,
  SZPS, /* set zp0, zp1, and zp2 to twilight zone 0 */

  PUSHB_1,
    3,
  CINDEX,
  PUSHB_1,
    sal_num_segments,
  RS,
  ADD, /* s: edge[-1] after edge before after_orig */
  PUSHB_1,
    2,
  CINDEX,
  PUSHB_1,
    sal_num_segments,
  RS,
  ADD, /* s: edge[-1] after edge before after_orig before_orig */
  MD_cur,
  PUSHB_1,
    0,
  EQ, /* after_orig_pos == before_orig_pos */
  IF, /* s: edge[-1] after edge before */
    MDAP_noround, /* set rp0 and rp1 to `before' */
    DUP,
    ALIGNRP, /* align `edge' with `before' */
    SWAP,
    POP,

  ELSE,
    PUSHB_1,
      2,
    CINDEX,
    PUSHB_1,
      sal_num_segments,
    RS,
    ADD, /* s: ... after edge before edge_orig */
    PUSHB_1,
      2,
    CINDEX,
    PUSHB_1,
      sal_num_segments,
    RS,
    ADD, /* s: ... after edge before edge_orig before_orig */
    MD_cur, /* a = edge_orig_pos - before_orig_pos */
    PUSHW_1,
      0x10, /* 64*64 */
      0x00,
    MUL,

    PUSHB_1,
      4,
    CINDEX, /* s: ... after edge before a*64 after */
    PUSHB_1,
      3,
    CINDEX, /* s: ... after edge before a*64 after before */
    MD_cur, /* b = after_pos - before_pos */
    MUL, /* s: ... after edge before a*b */

    PUSHB_1,
      4,
    CINDEX,
    PUSHB_1,
      sal_num_segments,
    RS,
    ADD, /* s: ... after edge before a*b after_orig */
    PUSHB_1,
      3,
    CINDEX,
    PUSHB_1,
      sal_num_segments,
    RS,
    ADD, /* s: ... after edge before a*b after_orig before_orig */
    MD_cur, /* c = after_orig_pos - before_orig_pos */
    PUSHW_1,
      0x10, /* 64*64 */
      0x00,
    MUL,

    DIV, /* s: edge[-1] after edge before a*b/c */

    SWAP,
    MDAP_noround, /* set rp0 and rp1 to `before' */
    SWAP, /* s: edge[-1] after a*b/c edge */
    DUP,
    DUP,
    ALIGNRP, /* align `edge' with `before' */
    ROLL,
    SHPIX, /* shift `edge' by `a*b/c' */

    SWAP, /* s: edge[-1] edge after */
    POP,
  EIF,

  SWAP, /* s: edge edge[-1] */
  DUP,
  MDAP_noround, /* set rp0 and rp1 to `edge[-1]' */
  GC_cur,
  PUSHB_1,
    2,
  CINDEX,
  GC_cur, /* s: edge edge[-1]_pos edge_pos */
  GT, /* edge_pos < edge[-1]_pos */
  IF,
    DUP,
    ALIGNRP, /* align `edge' to `edge[-1]' */
  EIF,

  MDAP_noround, /* set rp0 and rp1 to `edge' */

  PUSHB_2,
    bci_align_segments,
    1,
  SZP1, /* set zp1 to normal zone 1 */
  CALL,
  ENDF,

};


/*
 * bci_action_serif_link1_upper_bound
 *
 *   Handle the SERIF_LINK1 action to align a serif, depending on edges
 *   before and after.  Additionally, move the serif again if necessary to
 *   stay within an upper bound.
 *
 * in: before_point (in twilight zone)
 *     edge_point (in twilight zone)
 *     after_point (in twilight zone)
 *     edge[1] (in twilight zone)
 *     ... stuff for bci_align_segments (edge) ...
 */

unsigned char FPGM(bci_action_serif_link1_upper_bound) [] = {

  PUSHB_1,
    bci_action_serif_link1_upper_bound,
  FDEF,

  PUSHB_1,
    0,
  SZPS, /* set zp0, zp1, and zp2 to twilight zone 0 */

  PUSHB_1,
    3,
  CINDEX,
  PUSHB_1,
    sal_num_segments,
  RS,
  ADD, /* s: edge[1] after edge before after_orig */
  PUSHB_1,
    2,
  CINDEX,
  PUSHB_1,
    sal_num_segments,
  RS,
  ADD, /* s: edge[1] after edge before after_orig before_orig */
  MD_cur,
  PUSHB_1,
    0,
  EQ, /* after_orig_pos == before_orig_pos */
  IF, /* s: edge[1] after edge before */
    MDAP_noround, /* set rp0 and rp1 to `before' */
    DUP,
    ALIGNRP, /* align `edge' with `before' */
    SWAP,
    POP,

  ELSE,
    PUSHB_1,
      2,
    CINDEX,
    PUSHB_1,
      sal_num_segments,
    RS,
    ADD, /* s: ... after edge before edge_orig */
    PUSHB_1,
      2,
    CINDEX,
    PUSHB_1,
      sal_num_segments,
    RS,
    ADD, /* s: ... after edge before edge_orig before_orig */
    MD_cur, /* a = edge_orig_pos - before_orig_pos */
    PUSHW_1,
      0x10, /* 64*64 */
      0x00,
    MUL,

    PUSHB_1,
      4,
    CINDEX, /* s: ... after edge before a*64 after */
    PUSHB_1,
      3,
    CINDEX, /* s: ... after edge before a*64 after before */
    MD_cur, /* b = after_pos - before_pos */
    MUL, /* s: ... after edge before a*b */

    PUSHB_1,
      4,
    CINDEX,
    PUSHB_1,
      sal_num_segments,
    RS,
    ADD, /* s: ... after edge before a*b after_orig */
    PUSHB_1,
      3,
    CINDEX,
    PUSHB_1,
      sal_num_segments,
    RS,
    ADD, /* s: ... after edge before a*b after_orig before_orig */
    MD_cur, /* c = after_orig_pos - before_orig_pos */
    PUSHW_1,
      0x10, /* 64*64 */
      0x00,
    MUL,

    DIV, /* s: edge[1] after edge before a*b/c */

    SWAP,
    MDAP_noround, /* set rp0 and rp1 to `before' */
    SWAP, /* s: edge[1] after a*b/c edge */
    DUP,
    DUP,
    ALIGNRP, /* align `edge' with `before' */
    ROLL,
    SHPIX, /* shift `edge' by `a*b/c' */

    SWAP, /* s: edge[1] edge after */
    POP,
  EIF,

  SWAP, /* s: edge edge[1] */
  DUP,
  MDAP_noround, /* set rp0 and rp1 to `edge[1]' */
  GC_cur,
  PUSHB_1,
    2,
  CINDEX,
  GC_cur, /* s: edge edge[1]_pos edge_pos */
  LT, /* edge_pos > edge[1]_pos */
  IF,
    DUP,
    ALIGNRP, /* align `edge' to `edge[1]' */
  EIF,

  MDAP_noround, /* set rp0 and rp1 to `edge' */

  PUSHB_2,
    bci_align_segments,
    1,
  SZP1, /* set zp1 to normal zone 1 */
  CALL,

  ENDF,

};


/*
 * bci_action_serif_link1_lower_upper_bound
 *
 *   Handle the SERIF_LINK1 action to align a serif, depending on edges
 *   before and after.  Additionally, move the serif again if necessary to
 *   stay within a lower and upper bound.
 *
 * in: before_point (in twilight zone)
 *     edge_point (in twilight zone)
 *     after_point (in twilight zone)
 *     edge[-1] (in twilight zone)
 *     edge[1] (in twilight zone)
 *     ... stuff for bci_align_segments (edge) ...
 */

unsigned char FPGM(bci_action_serif_link1_lower_upper_bound) [] = {

  PUSHB_1,
    bci_action_serif_link1_lower_upper_bound,
  FDEF,

  PUSHB_1,
    0,
  SZPS, /* set zp0, zp1, and zp2 to twilight zone 0 */

  PUSHB_1,
    3,
  CINDEX,
  PUSHB_1,
    sal_num_segments,
  RS,
  ADD, /* s: edge[1] edge[-1] after edge before after_orig */
  PUSHB_1,
    2,
  CINDEX,
  PUSHB_1,
    sal_num_segments,
  RS,
  ADD, /* s: edge[1] edge[-1] after edge before after_orig before_orig */
  MD_cur,
  PUSHB_1,
    0,
  EQ, /* after_orig_pos == before_orig_pos */
  IF, /* s: edge[1] edge[-1] after edge before */
    MDAP_noround, /* set rp0 and rp1 to `before' */
    DUP,
    ALIGNRP, /* align `edge' with `before' */
    SWAP,
    POP,

  ELSE,
    PUSHB_1,
      2,
    CINDEX,
    PUSHB_1,
      sal_num_segments,
    RS,
    ADD, /* s: ... after edge before edge_orig */
    PUSHB_1,
      2,
    CINDEX,
    PUSHB_1,
      sal_num_segments,
    RS,
    ADD, /* s: ... after edge before edge_orig before_orig */
    MD_cur, /* a = edge_orig_pos - before_orig_pos */
    PUSHW_1,
      0x10, /* 64*64 */
      0x00,
    MUL,

    PUSHB_1,
      4,
    CINDEX, /* s: ... after edge before a*64 after */
    PUSHB_1,
      3,
    CINDEX, /* s: ... after edge before a*64 after before */
    MD_cur, /* b = after_pos - before_pos */
    MUL, /* s: ... after edge before a*b */

    PUSHB_1,
      4,
    CINDEX,
    PUSHB_1,
      sal_num_segments,
    RS,
    ADD, /* s: ... after edge before a*b after_orig */
    PUSHB_1,
      3,
    CINDEX,
    PUSHB_1,
      sal_num_segments,
    RS,
    ADD, /* s: ... after edge before a*b after_orig before_orig */
    MD_cur, /* c = after_orig_pos - before_orig_pos */
    PUSHW_1,
      0x10, /* 64*64 */
      0x00,
    MUL,

    DIV, /* s: edge[1] edge[-1] after edge before a*b/c */

    SWAP,
    MDAP_noround, /* set rp0 and rp1 to `before' */
    SWAP, /* s: edge[1] edge[-1] after a*b/c edge */
    DUP,
    DUP,
    ALIGNRP, /* align `edge' with `before' */
    ROLL,
    SHPIX, /* shift `edge' by `a*b/c' */

    SWAP, /* s: edge[1] edge[-1] edge after */
    POP,
  EIF,

  SWAP, /* s: edge[1] edge edge[-1] */
  DUP,
  MDAP_noround, /* set rp0 and rp1 to `edge[-1]' */
  GC_cur,
  PUSHB_1,
    2,
  CINDEX,
  GC_cur, /* s: edge[1] edge edge[-1]_pos edge_pos */
  GT, /* edge_pos < edge[-1]_pos */
  IF,
    DUP,
    ALIGNRP, /* align `edge' to `edge[-1]' */
  EIF,

  SWAP, /* s: edge edge[1] */
  DUP,
  MDAP_noround, /* set rp0 and rp1 to `edge[1]' */
  GC_cur,
  PUSHB_1,
    2,
  CINDEX,
  GC_cur, /* s: edge edge[1]_pos edge_pos */
  LT, /* edge_pos > edge[1]_pos */
  IF,
    DUP,
    ALIGNRP, /* align `edge' to `edge[1]' */
  EIF,

  MDAP_noround, /* set rp0 and rp1 to `edge' */

  PUSHB_2,
    bci_align_segments,
    1,
  SZP1, /* set zp1 to normal zone 1 */
  CALL,

  ENDF,

};


/*
 * bci_action_serif_link2
 *
 *   Handle the SERIF_LINK2 action to align a serif relative to the anchor.
 *
 * in: edge_point (in twilight zone)
 *     ... stuff for bci_align_segments (edge) ...
 */

unsigned char FPGM(bci_action_serif_link2) [] = {

  PUSHB_1,
    bci_action_serif_link2,
  FDEF,

  PUSHB_1,
    0,
  SZPS, /* set zp0, zp1, and zp2 to twilight zone 0 */

  DUP,
  PUSHB_1,
    sal_num_segments,
  RS,
  ADD, /* s: edge edge_orig */
  PUSHB_1,
    sal_anchor,
  RS,
  DUP,
  MDAP_noround, /* set rp0 and rp1 to `sal_anchor' */
  PUSHB_1,
    sal_num_segments,
  RS,
  ADD, /* s: edge edge_orig anchor_orig */

  MD_cur,
  DUP,
  ADD,
  PUSHB_1,
    32,
  ADD,
  FLOOR,
  PUSHB_1,
    2*64,
  DIV, /* delta = (edge_orig_pos - anchor_orig_pos + 16) & ~31 */

  SWAP,
  DUP,
  DUP,
  ALIGNRP, /* align `edge' with `sal_anchor' */
  ROLL,
  SHPIX, /* shift `edge' by `delta' */

  MDAP_noround, /* set rp0 and rp1 to `edge' */

  PUSHB_2,
    bci_align_segments,
    1,
  SZP1, /* set zp1 to normal zone 1 */
  CALL,

  ENDF,

};


/*
 * bci_action_serif_link2_lower_bound
 *
 *   Handle the SERIF_LINK2 action to align a serif relative to the anchor.
 *   Additionally, move the serif again if necessary to stay within a lower
 *   bound.
 *
 * in: edge_point (in twilight zone)
 *     edge[-1] (in twilight zone)
 *     ... stuff for bci_align_segments (edge) ...
 */

unsigned char FPGM(bci_action_serif_link2_lower_bound) [] = {

  PUSHB_1,
    bci_action_serif_link2_lower_bound,
  FDEF,

  PUSHB_1,
    0,
  SZPS, /* set zp0, zp1, and zp2 to twilight zone 0 */

  DUP,
  PUSHB_1,
    sal_num_segments,
  RS,
  ADD, /* s: edge[-1] edge edge_orig */
  PUSHB_1,
    sal_anchor,
  RS,
  DUP,
  MDAP_noround, /* set rp0 and rp1 to `sal_anchor' */
  PUSHB_1,
    sal_num_segments,
  RS,
  ADD, /* s: edge[-1] edge edge_orig anchor_orig */

  MD_cur,
  DUP,
  ADD,
  PUSHB_1,
    32,
  ADD,
  FLOOR,
  PUSHB_1,
    2*64,
  DIV, /* delta = (edge_orig_pos - anchor_orig_pos + 16) & ~31 */

  SWAP,
  DUP,
  DUP,
  ALIGNRP, /* align `edge' with `sal_anchor' */
  ROLL,
  SHPIX, /* shift `edge' by `delta' */

  SWAP, /* s: edge edge[-1] */
  DUP,
  MDAP_noround, /* set rp0 and rp1 to `edge[-1]' */
  GC_cur,
  PUSHB_1,
    2,
  CINDEX,
  GC_cur, /* s: edge edge[-1]_pos edge_pos */
  GT, /* edge_pos < edge[-1]_pos */
  IF,
    DUP,
    ALIGNRP, /* align `edge' to `edge[-1]' */
  EIF,

  MDAP_noround, /* set rp0 and rp1 to `edge' */

  PUSHB_2,
    bci_align_segments,
    1,
  SZP1, /* set zp1 to normal zone 1 */
  CALL,

  ENDF,

};


/*
 * bci_action_serif_link2_upper_bound
 *
 *   Handle the SERIF_LINK2 action to align a serif relative to the anchor.
 *   Additionally, move the serif again if necessary to stay within an upper
 *   bound.
 *
 * in: edge_point (in twilight zone)
 *     edge[1] (in twilight zone)
 *     ... stuff for bci_align_segments (edge) ...
 */

unsigned char FPGM(bci_action_serif_link2_upper_bound) [] = {

  PUSHB_1,
    bci_action_serif_link2_upper_bound,
  FDEF,

  PUSHB_1,
    0,
  SZPS, /* set zp0, zp1, and zp2 to twilight zone 0 */

  DUP,
  PUSHB_1,
    sal_num_segments,
  RS,
  ADD, /* s: edge[1] edge edge_orig */
  PUSHB_1,
    sal_anchor,
  RS,
  DUP,
  MDAP_noround, /* set rp0 and rp1 to `sal_anchor' */
  PUSHB_1,
    sal_num_segments,
  RS,
  ADD, /* s: edge[1] edge edge_orig anchor_orig */

  MD_cur,
  DUP,
  ADD,
  PUSHB_1,
    32,
  ADD,
  FLOOR,
  PUSHB_1,
    2*64,
  DIV, /* delta = (edge_orig_pos - anchor_orig_pos + 16) & ~31 */

  SWAP,
  DUP,
  DUP,
  ALIGNRP, /* align `edge' with `sal_anchor' */
  ROLL,
  SHPIX, /* shift `edge' by `delta' */

  SWAP, /* s: edge edge[1] */
  DUP,
  MDAP_noround, /* set rp0 and rp1 to `edge[1]' */
  GC_cur,
  PUSHB_1,
    2,
  CINDEX,
  GC_cur, /* s: edge edge[1]_pos edge_pos */
  LT, /* edge_pos > edge[1]_pos */
  IF,
    DUP,
    ALIGNRP, /* align `edge' to `edge[1]' */
  EIF,

  MDAP_noround, /* set rp0 and rp1 to `edge' */

  PUSHB_2,
    bci_align_segments,
    1,
  SZP1, /* set zp1 to normal zone 1 */
  CALL,

  ENDF,

};


/*
 * bci_action_serif_link2_lower_upper_bound
 *
 *   Handle the SERIF_LINK2 action to align a serif relative to the anchor.
 *   Additionally, move the serif again if necessary to stay within a lower
 *   and upper bound.
 *
 * in: edge_point (in twilight zone)
 *     edge[-1] (in twilight zone)
 *     edge[1] (in twilight zone)
 *     ... stuff for bci_align_segments (edge) ...
 */

unsigned char FPGM(bci_action_serif_link2_lower_upper_bound) [] = {

  PUSHB_1,
    bci_action_serif_link2_lower_upper_bound,
  FDEF,

  PUSHB_1,
    0,
  SZPS, /* set zp0, zp1, and zp2 to twilight zone 0 */

  DUP,
  PUSHB_1,
    sal_num_segments,
  RS,
  ADD, /* s: edge[1] edge[-1] edge edge_orig */
  PUSHB_1,
    sal_anchor,
  RS,
  DUP,
  MDAP_noround, /* set rp0 and rp1 to `sal_anchor' */
  PUSHB_1,
    sal_num_segments,
  RS,
  ADD, /* s: edge[1] edge[-1] edge edge_orig anchor_orig */

  MD_cur,
  DUP,
  ADD,
  PUSHB_1,
    32,
  ADD,
  FLOOR,
  PUSHB_1,
    2*64,
  DIV, /* delta = (edge_orig_pos - anchor_orig_pos + 16) & ~31 */

  SWAP,
  DUP,
  DUP,
  ALIGNRP, /* align `edge' with `sal_anchor' */
  ROLL,
  SHPIX, /* shift `edge' by `delta' */

  SWAP, /* s: edge[1] edge edge[-1] */
  DUP,
  MDAP_noround, /* set rp0 and rp1 to `edge[-1]' */
  GC_cur,
  PUSHB_1,
    2,
  CINDEX,
  GC_cur, /* s: edge[1] edge edge[-1]_pos edge_pos */
  GT, /* edge_pos < edge[-1]_pos */
  IF,
    DUP,
    ALIGNRP, /* align `edge' to `edge[-1]' */
  EIF,

  SWAP, /* s: edge edge[1] */
  DUP,
  MDAP_noround, /* set rp0 and rp1 to `edge[1]' */
  GC_cur,
  PUSHB_1,
    2,
  CINDEX,
  GC_cur, /* s: edge edge[1]_pos edge_pos */
  LT, /* edge_pos > edge[1]_pos */
  IF,
    DUP,
    ALIGNRP, /* align `edge' to `edge[1]' */
  EIF,

  MDAP_noround, /* set rp0 and rp1 to `edge' */

  PUSHB_2,
    bci_align_segments,
    1,
  SZP1, /* set zp1 to normal zone 1 */
  CALL,

  ENDF,

};


/*
 * bci_handle_action
 *
 *   Execute function.
 *
 * in: function_index
 */

unsigned char FPGM(bci_handle_action) [] = {

  PUSHB_1,
    bci_handle_action,
  FDEF,

  CALL,

  ENDF,

};


/*
 * bci_hint_glyph
 *
 *   This is the top-level glyph hinting function
 *   which parses the arguments on the stack and calls subroutines.
 *
 * in: num_actions (M)
 *       action_0_func_idx
 *         ... data ...
 *       action_1_func_idx
 *         ... data ...
 *       ...
 *       action_M_func_idx
 *         ... data ...
 *
 * uses: bci_handle_action
 *       bci_action_adjust_bound
 *       bci_action_stem_bound
 *
 *       bci_action_link
 *       bci_action_anchor
 *       bci_action_blue_anchor
 *       bci_action_adjust
 *       bci_action_stem
 *
 *       bci_action_blue
 *       bci_action_serif
 *       bci_action_serif_anchor
 *       bci_action_serif_link1
 *       bci_action_serif_link2
 */

unsigned char FPGM(bci_hint_glyph) [] = {

  PUSHB_1,
    bci_hint_glyph,
  FDEF,

  PUSHB_1,
    bci_handle_action,
  LOOPCALL,

  IUP_y,

  ENDF,

};


#define COPY_FPGM(func_name) \
          memcpy(buf_p, fpgm_ ## func_name, \
                 sizeof (fpgm_ ## func_name)); \
          buf_p += sizeof (fpgm_ ## func_name) \

static FT_Error
TA_table_build_fpgm(FT_Byte** fpgm,
                    FT_ULong* fpgm_len,
                    FONT* font)
{
  FT_UInt buf_len;
  FT_UInt len;
  FT_Byte* buf;
  FT_Byte* buf_p;


  buf_len = sizeof (FPGM(bci_round))
            + sizeof (FPGM(bci_compute_stem_width_a))
            + 1
            + sizeof (FPGM(bci_compute_stem_width_b))
            + 1
            + sizeof (FPGM(bci_compute_stem_width_c))
            + sizeof (FPGM(bci_loop))
            + sizeof (FPGM(bci_cvt_rescale))
            + sizeof (FPGM(bci_blue_round_a))
            + 1
            + sizeof (FPGM(bci_blue_round_b))
            + sizeof (FPGM(bci_get_point_extrema))

            + sizeof (FPGM(bci_create_segment))
            + sizeof (FPGM(bci_create_segments))
            + sizeof (FPGM(bci_align_segment))
            + sizeof (FPGM(bci_align_segments))

            + sizeof (FPGM(bci_scale_contour))
            + sizeof (FPGM(bci_scale_glyph))
            + sizeof (FPGM(bci_shift_contour))
            + sizeof (FPGM(bci_shift_subglyph))

            + sizeof (FPGM(bci_ip_outer_align_point))
            + sizeof (FPGM(bci_ip_on_align_points))
            + sizeof (FPGM(bci_ip_between_align_point))
            + sizeof (FPGM(bci_ip_between_align_points))

            + sizeof (FPGM(bci_action_ip_before))
            + sizeof (FPGM(bci_action_ip_after))
            + sizeof (FPGM(bci_action_ip_on))
            + sizeof (FPGM(bci_action_ip_between))

            + sizeof (FPGM(bci_action_adjust_bound))
            + sizeof (FPGM(bci_action_stem_bound))
            + sizeof (FPGM(bci_action_link))
            + sizeof (FPGM(bci_action_anchor))
            + sizeof (FPGM(bci_action_blue_anchor))
            + sizeof (FPGM(bci_action_adjust))
            + sizeof (FPGM(bci_action_stem))
            + sizeof (FPGM(bci_action_blue))
            + sizeof (FPGM(bci_action_serif))
            + sizeof (FPGM(bci_action_serif_lower_bound))
            + sizeof (FPGM(bci_action_serif_upper_bound))
            + sizeof (FPGM(bci_action_serif_lower_upper_bound))
            + sizeof (FPGM(bci_action_serif_anchor))
            + sizeof (FPGM(bci_action_serif_anchor_lower_bound))
            + sizeof (FPGM(bci_action_serif_anchor_upper_bound))
            + sizeof (FPGM(bci_action_serif_anchor_lower_upper_bound))
            + sizeof (FPGM(bci_action_serif_link1))
            + sizeof (FPGM(bci_action_serif_link1_lower_bound))
            + sizeof (FPGM(bci_action_serif_link1_upper_bound))
            + sizeof (FPGM(bci_action_serif_link1_lower_upper_bound))
            + sizeof (FPGM(bci_action_serif_link2))
            + sizeof (FPGM(bci_action_serif_link2_lower_bound))
            + sizeof (FPGM(bci_action_serif_link2_upper_bound))
            + sizeof (FPGM(bci_action_serif_link2_lower_upper_bound))

            + sizeof (FPGM(bci_handle_action))
            + sizeof (FPGM(bci_hint_glyph));
  /* buffer length must be a multiple of four */
  len = (buf_len + 3) & ~3;
  buf = (FT_Byte*)malloc(len);
  if (!buf)
    return FT_Err_Out_Of_Memory;

  /* pad end of buffer with zeros */
  buf[len - 1] = 0x00;
  buf[len - 2] = 0x00;
  buf[len - 3] = 0x00;

  /* copy font program into buffer and fill in the missing variables */
  buf_p = buf;

  COPY_FPGM(bci_round);
  COPY_FPGM(bci_compute_stem_width_a);
  *(buf_p++) = (unsigned char)CVT_VERT_WIDTHS_OFFSET(font);
  COPY_FPGM(bci_compute_stem_width_b);
  *(buf_p++) = (unsigned char)CVT_VERT_WIDTHS_OFFSET(font);
  COPY_FPGM(bci_compute_stem_width_c);
  COPY_FPGM(bci_loop);
  COPY_FPGM(bci_cvt_rescale);
  COPY_FPGM(bci_blue_round_a);
  *(buf_p++) = (unsigned char)CVT_BLUES_SIZE(font);
  COPY_FPGM(bci_blue_round_b);
  COPY_FPGM(bci_get_point_extrema);

  COPY_FPGM(bci_create_segment);
  COPY_FPGM(bci_create_segments);
  COPY_FPGM(bci_align_segment);
  COPY_FPGM(bci_align_segments);

  COPY_FPGM(bci_scale_contour);
  COPY_FPGM(bci_scale_glyph);
  COPY_FPGM(bci_shift_contour);
  COPY_FPGM(bci_shift_subglyph);

  COPY_FPGM(bci_ip_outer_align_point);
  COPY_FPGM(bci_ip_on_align_points);
  COPY_FPGM(bci_ip_between_align_point);
  COPY_FPGM(bci_ip_between_align_points);

  COPY_FPGM(bci_action_ip_before);
  COPY_FPGM(bci_action_ip_after);
  COPY_FPGM(bci_action_ip_on);
  COPY_FPGM(bci_action_ip_between);

  COPY_FPGM(bci_action_adjust_bound);
  COPY_FPGM(bci_action_stem_bound);
  COPY_FPGM(bci_action_link);
  COPY_FPGM(bci_action_anchor);
  COPY_FPGM(bci_action_blue_anchor);
  COPY_FPGM(bci_action_adjust);
  COPY_FPGM(bci_action_stem);
  COPY_FPGM(bci_action_blue);
  COPY_FPGM(bci_action_serif);
  COPY_FPGM(bci_action_serif_lower_bound);
  COPY_FPGM(bci_action_serif_upper_bound);
  COPY_FPGM(bci_action_serif_lower_upper_bound);
  COPY_FPGM(bci_action_serif_anchor);
  COPY_FPGM(bci_action_serif_anchor_lower_bound);
  COPY_FPGM(bci_action_serif_anchor_upper_bound);
  COPY_FPGM(bci_action_serif_anchor_lower_upper_bound);
  COPY_FPGM(bci_action_serif_link1);
  COPY_FPGM(bci_action_serif_link1_lower_bound);
  COPY_FPGM(bci_action_serif_link1_upper_bound);
  COPY_FPGM(bci_action_serif_link1_lower_upper_bound);
  COPY_FPGM(bci_action_serif_link2);
  COPY_FPGM(bci_action_serif_link2_lower_bound);
  COPY_FPGM(bci_action_serif_link2_upper_bound);
  COPY_FPGM(bci_action_serif_link2_lower_upper_bound);

  COPY_FPGM(bci_handle_action);
  COPY_FPGM(bci_hint_glyph);

  *fpgm = buf;
  *fpgm_len = buf_len;

  return FT_Err_Ok;
}


FT_Error
TA_sfnt_build_fpgm_table(SFNT* sfnt,
                         FONT* font)
{
  FT_Error error;

  FT_Byte* fpgm_buf;
  FT_ULong fpgm_len;


  error = TA_sfnt_add_table_info(sfnt);
  if (error)
    return error;

  error = TA_table_build_fpgm(&fpgm_buf, &fpgm_len, font);
  if (error)
    return error;

  /* in case of success, `fpgm_buf' gets linked */
  /* and is eventually freed in `TA_font_unload' */
  error = TA_font_add_table(font,
                            &sfnt->table_infos[sfnt->num_table_infos - 1],
                            TTAG_fpgm, fpgm_len, fpgm_buf);
  if (error)
  {
    free(fpgm_buf);
    return error;
  }

  return FT_Err_Ok;
}

/* end of tafpgm.c */
