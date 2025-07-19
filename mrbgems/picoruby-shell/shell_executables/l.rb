puts "LED test"
pin = GPIO.new(25, GPIO::OUT)

100.times do
  #puts "LED ON"
  pin.write 1
  sleep 0.5
  #puts "LED OFF"
  pin.write 0
  sleep 0.5
end

puts "L chika done"

