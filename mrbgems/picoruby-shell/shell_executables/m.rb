puts "start test"
pin = GPIO.new(25, GPIO::OUT)

3.times do
  puts "loop"
  pin.write 1
  sleep 0.5
  pin.write 0
  sleep 0.5
end

puts "L chika done"

require "i2c"
puts "start m"

class JHD1802
    def initialize(address = 0x3E)
        puts "Initialize start"
        @addr = address
        @bus = nil
        
        begin
            puts "Creating I2C instance..."
            @bus = I2C.new(unit: :RP2040_I2C1, sda_pin: 2, scl_pin: 3)
            puts "I2C instance created"
            puts "Testing device connection..."
            @bus.scan
            @bus.write(@addr, 0)
            puts "Device connected successfully"
        rescue IOError => e
            puts "IOError: #{e.message}"
            puts "Check if the I2C device inserted, then try again"
            return
        rescue => e
            puts "Unexpected error: #{e.message}"
            return
        end

        # Initialize LCD
        puts "init LCD"
        text_command(0x02)        # Home cursor
        sleep(0.1)
        text_command(0x08 | 0x04) # Display on, no cursor
        text_command(0x28)        # 2 lines, 5x8 font
    end

    def name
        "JHD1802"
    end

    def size
        [2, 16]  # [rows, columns]
    end

    def clear
        text_command(0x01) if @bus
    end

    def home
        return unless @bus
        text_command(0x02)
        sleep(0.2)
    end

    def set_cursor(row, column)
        return unless @bus
        text_command((0x40 * row) + (column % 0x10) + 0x80)
    end

    def write(msg)
        return unless @bus
        msg.each_char do |c|
            @bus.write(@addr, 0x40, c.ord)
        end
    end

    def cursor_on(enable)
        return unless @bus
        if enable
            text_command(0x0E)
        else
            text_command(0x0C)
        end
    end

    def text_command(cmd)
        @bus.write(@addr, 0x80, cmd) if @bus
    end
end

puts "JHD1802.new"

begin
    puts "Before I2C init"
    lcd = JHD1802.new
    puts "JHD1802.new done"
rescue => e
    puts "Error during init: #{e.message}"
end

#     rows, cols = lcd.size
#     puts "LCD model: #{lcd.name}"
#     puts "LCD type : #{cols} x #{rows}"

#     lcd.set_cursor(0, 0)
#     lcd.write("hello world!")
#     lcd.set_cursor(0, cols - 1)
#     lcd.write('X')
#     lcd.set_cursor(rows - 1, 0)
#     (0...cols).each do |i|
#         lcd.write(('A'.ord + i).chr)
#     end

#     sleep(3)
#     lcd.clear
# rescue => e
#     puts "Error: #{e.message}"
# end