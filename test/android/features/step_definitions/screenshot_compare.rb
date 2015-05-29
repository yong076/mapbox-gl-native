require 'oily_png'
require 'open-uri'
include ChunkyPNG::Color

def starts_with(item, prefix)
  prefix = prefix.to_s
  item[0, prefix.length] == prefix
end

# compares two images on disk, returns the % difference
def compare_image(image1, image2)
  images = [
    ChunkyPNG::Image.from_file("screens/#{image1}"),
    ChunkyPNG::Image.from_file("screens/#{image2}")
  ]
  count=0
  images.first.height.times do |y|
    images.first.row(y).each_with_index do |pixel, x|

      images.last[x,y] = rgb(
        r(pixel) + r(images.last[x,y]) - 2 * [r(pixel), r(images.last[x,y])].min,
        g(pixel) + g(images.last[x,y]) - 2 * [g(pixel), g(images.last[x,y])].min,
        b(pixel) + b(images.last[x,y]) - 2 * [b(pixel), b(images.last[x,y])].min
      )
      if images.last[x,y] == 255
        count = count + 1
      end
    end
  end

  100 - ((count.to_f / images.last.pixels.length.to_f) * 100);
end

# find the file
def get_screenshot_name(folder, fileName)
  foundName = fileName
  Dir.foreach('screens/') do |item|
  next if item == '.' or item == '..'
    if item.start_with? fileName.split('.')[0]
      foundName = item
    end
  end

  foundName
end

def setup_comparison(fileName, percentageVariance, forNotCase = false)
  screenshotFileName = "compare_#{fileName}"
  screenshot({ :prefix => "screens/", :name => screenshotFileName })

  screenshotFileName = get_screenshot_name("screens/", screenshotFileName)
  changed = compare_image(fileName, screenshotFileName)
  FileUtils.rm("screens/#{screenshotFileName}")

  assert = true
  if forNotCase
    assert = changed.to_i < percentageVariance
  else
    assert = changed.to_i > percentageVariance
  end

  if assert
    fail(msg="Error. The screen shot was different from the source file. Difference: #{changed.to_i}%")
  end

end

def setup_comparison_url(url, percentageVariance)
  fileName = "tester.png"
  open("screens/#{fileName}", 'wb') do |file|
    file << open(url).read
  end

  setup_comparison(fileName, percentageVariance)
  FileUtils.rm("screens/#{fileName}")
end

Then(/^I compare the screen with "(.*?)"$/) do |fileName|
  setup_comparison(fileName, 0)
end

Then(/^I compare the screen with url "(.*?)"$/) do |url|
  setup_comparison_url(url, 0)
end

Then(/^the screen should not match with "(.*?)"$/) do |fileName|
  setup_comparison(fileName, 0, true)
end

Then(/^I expect atmost "(.*?)" difference when comparing with "(.*?)"$/) do |percentageVariance, fileName|
  setup_comparison(fileName, percentageVariance.to_i)
end

Then(/^I expect atmost "(.*?)" difference when comparing with url "(.*?)"$/) do |percentageVariance, url|
  setup_comparison_url(url, percentageVariance.to_i)
end
