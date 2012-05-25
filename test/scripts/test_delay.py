#!/usr/bin/env python
# 
# Copyright (c) 2012, Stephan Wirth <stephan.wirth@gmail.com>
# All rights reserved.
# 
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
#     * Redistributions of source code must retain the above copyright
#       notice, this list of conditions and the following disclaimer.
#     * Redistributions in binary form must reproduce the above copyright
#       notice, this list of conditions and the following disclaimer in the
#       documentation and/or other materials provided with the distribution.
# 
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
# LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
# CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
# SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
# INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
# CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
# ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
# POSSIBILITY OF SUCH DAMAGE.
# 

import ecto
import ecto.ecto_test as ecto_test

def test_delay():
    plasm = ecto.Plasm()
    gen = ecto_test.Generate("Generator", step=1.0, start=0.0)
    delay = ecto.Delay(num=3)
    plasm.connect(gen['out'] >> delay['in']
                  )
    for i in range(10):
      plasm.execute(niter=1)
      if i < 3:
        assert delay.outputs.out == None
      else:
        assert delay.outputs.out == i - 3

def test_delay2():
    plasm = ecto.Plasm()
    gen = ecto_test.Generate("Generator", step=1.0, start=0.0)
    delay = ecto.Delay(num=1)
    adder = ecto_test.Add()
    plasm.connect(gen['out'] >> delay['in'],
                  delay['out'] >> adder['left'],
                  gen['out'] >> adder['right'],
                  )
    for i in range(10):
      plasm.execute(niter=1)
      if i > 0:
        assert adder.outputs.out == i + (i-1)

 
if __name__ == '__main__':
    test_delay()
    test_delay2()

