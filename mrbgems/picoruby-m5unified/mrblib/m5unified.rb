# M5Unified Ruby wrapper for PicoRuby
class M5
  class << self
    # Initialize M5 hardware
    # @return [void]
    def begin
      # Implemented in C
      puts "begin"
      _begin()
    end

    # Update M5 state (call this in main loop)
    # @return [void]
    def update
      # Implemented in C
      puts "update"
      _update()
    end
  end

  # Display control class
  class Display
    class << self
      # Clear the display
      # @return [void]
      def clear
        puts "clear"
        _clear()
        # Implemented in C
      end

      # # Print text to display
      # # @param text [String] text to print
      # # @return [void]
      # def print(text)
      #   # Implemented in C
      # end

      # # Print text with newline to display
      # # @param text [String] text to print
      # # @return [void]
      # def println(text)
      #   # Implemented in C
      # end

      # # Draw a pixel at specified coordinates
      # # @param x [Integer] x coordinate
      # # @param y [Integer] y coordinate
      # # @param color [Integer] color value
      # # @return [void]
      # def draw_pixel(x, y, color)
      #   # Implemented in C
      # end

      # # Fill entire screen with color
      # # @param color [Integer] color value
      # # @return [void]
      # def fill_screen(color)
      #   # Implemented in C
      # end
    end
  end

  # Button A control class
  class BtnA
    class << self
      # Check if button A is pressed
      # @return [Boolean] true if pressed, false otherwise
      def pressed?
        # Implemented in C
      end
    end
  end

  # Button B control class
  class BtnB
    class << self
      # Check if button B is pressed
      # @return [Boolean] true if pressed, false otherwise
      def pressed?
        # Implemented in C
      end
    end
  end

  # Button C control class
  class BtnC
    class << self
      # Check if button C is pressed
      # @return [Boolean] true if pressed, false otherwise
      def pressed?
        # Implemented in C
      end
    end
  end
end