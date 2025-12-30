# Scrollbar position limits

QScrollBar uses an int32_t for positions.  We're using frame counts for the
position.  This means the scrollbar's position will overflow if there are more
than 2**31 frames, which is a little over 12 hours at 48kHz.  The user could
realistically want to record and process more than 12 hours of audio.

For now I'm limiting maximum frames to 2**31.  Options to remove the limit:
  - Use strides
    - Requires recomputing the scrollbar when stride changes.
    - Will still overflow in a vaguely plausible buffer size
  - Just divide by a fixed value
    - Might limit the ability to precisely seek when using small transforms / large stride
    - Will still overflow in a vaguely plausible buffer size
