task :default => [:doc]

require 'rubygems'
require 'bluecloth'
require 'find'
require 'fileutils'

SKUG_VERSION = "0.1.0"

DocumentWrapper = %{
<html>
  <head><title>%s</title></head>
  <body>
%s
  </body>
</html>
}

def md_to_html(input, output, title)
  contents = File::readlines(input, nil)
	bc = BlueCloth::new(contents.join)
	puts "#{input} -> #{output} (#{title})"
	File.open(output, "w") { |io| io.puts DocumentWrapper % [title, bc.to_html] }
end

def with_doc_files(dir)
  Find.find(".") { |file|
    if /(.*)\.md$/ =~ file then
      outfile = file == "./README.md" ? $1 + ".html" : $1
      yield(file, outfile)
    end
  }
end

task :doc do
  Find.find(".") { |file|
    if /(.*)\.md$/ =~ file then
      output = file == "./README.md" ? $1 + ".html" : $1
      title = File.basename($1)
      md_to_html(file, output, title)
    end
  }
end

task [:doc, :clean] do
  with_doc_files(".") { |infile,outfile|
    FileUtils.rm(outfile)
  }
end

def xcodebuild(project, command, target="All", config="Deployment")
  system("xcodebuild", "-project", project, "-target", target, "-configuration", config, command)
end

XCODEPROJ = "skUG.xcodeproj"

task :xcode do
  xcodebuild(XCODEPROJ, "build")
end

task [:xcode, :clean] do
  xcodebuild(XCODEPROJ, "clean")
end

task [:dist, :bin] do
  dist_files = ["skUG/FM7"]
  dist_dir = "skUG-bin-#{SKUG_VERSION}"
  `mkdir -p #{dist_dir}`
  dist_files.each { |f|
    `rsync -av #{f} #{dist_dir}`
  }
  system("tar", "vvcfz", "#{dist_dir}.tar.gz", dist_dir)
  `rm -r #{dist_dir}`
end
