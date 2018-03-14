CC=gcc
CFLAGS=-Wall -O2 -g -fomit-frame-pointer -funroll-loops -finline-functions -ffast-math -I../src

OS=$(shell uname)
ifeq ($(OS),Darwin)
  DSSI_CFLAGS=`pkg-config dssi libdssialsacompat --cflags`
  LDFLAGS=-lm `pkg-config dssi libdssialsacompat --libs`
else
  DSSI_CFLAGS=`pkg-config dssi alsa --cflags`
  LDFLAGS=-lm -lpthread `pkg-config dssi alsa --libs`
endif

DEPS = wrapper.h ../src/dx7_voice.h ../src/dx7_voice_data.h ../src/hexter.h \
    ../src/hexter_synth.h ../src/hexter_types.h

OBJ = dx7_voice_fix.o dx7_voice_data_fix.o \
    dx7_voice_render_fix.o dx7_voice_tables_fix.o \
    hexter_fix.o hexter_synth_fix.o \
    dx7_voice_float.o dx7_voice_data_float.o \
    dx7_voice_render_float.o dx7_voice_tables_float.o \
    hexter_float.o hexter_synth_float.o \
    dx7_voice_patches.o \
    harness.o

%_fix.o: ../src/%.c $(DEPS)
	$(CC) $(CFLAGS) $(DSSI_CFLAGS) -c -o $@ $< -include wrapper.h

%_float.o: ../src/%.c $(DEPS)
	$(CC) $(CFLAGS) $(DSSI_CFLAGS) -DHEXTER_USE_FLOATING_POINT -c -o $@ $< -include wrapper.h

%.o: ../src/%.c $(DEPS)
	$(CC) $(CFLAGS) $(DSSI_CFLAGS) -c -o $@ $<

fptest: $(OBJ)
	$(CC) -o $@ $^ $(LDFLAGS)

harness.o: harness.c $(DEPS)
	$(CC) $(CFLAGS) $(DSSI_CFLAGS) -c -o $@ $<

.PHONY: clean

clean:
	rm fptest *.o

