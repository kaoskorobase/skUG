task :default => [:doc]

require 'rubygems'
require 'bluecloth'
require 'find'
require 'fileutils'

PACKAGE      = "skUG"
SKUG_VERSION = File.read("VERSION").strip
SRC_DIST     = "#{PACKAGE}-src-#{SKUG_VERSION}"
BIN_DIST     = "#{PACKAGE}-bin-#{SKUG_VERSION}"

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
    if ["CVS", "_darcs", ".svn"].include?(File.basename(File.dirname(file)))
      Find.prune
    else
      if /(.*)\.md$/ =~ file then
        # outfile = file == "./README.md" ? $1 + ".html" : $1
        outfile = $1 + ".html"
        yield(file, outfile)
      end
    end
  }
end

def xcodebuild(project, command, target="All", config="Deployment")
  system("xcodebuild", "-project", project, "-target", target, "-configuration", config, command)
end

# =====================================================================
# Building

XCODEPROJ = "skUG.xcodeproj"

task :xcode do
  xcodebuild(XCODEPROJ, "build")
end

task [:xcode, :clean] do
  xcodebuild(XCODEPROJ, "clean")
end

# =====================================================================
# Documentation

task :doc do
  with_doc_files(".") { |infile,outfile|
    title = File.readlines(infile)[0].strip.sub(/^# /, '')
    md_to_html(infile, outfile, title)
  }
end

task [:doc, :clean] do
  with_doc_files(".") { |infile,outfile|
    FileUtils.rm(outfile)
  }
end


# =====================================================================
# Distribution

task [:dist, :src] do
  `darcs dist -d #{SRC_DIST}`
end

task [:dist, :bin] do
  dist_files = ["skUG/FM7"]
  dist_dir = BIN_DIST
  `mkdir -p #{dist_dir}`
  dist_files.each { |f|
    `rsync -a #{f} #{dist_dir}`
  }
  system("tar", "cfz", "#{dist_dir}.tar.gz", dist_dir)
  `rm -r #{dist_dir}`
end

task :dist => ["dist:src", "dist:bin"]

# =====================================================================
# Cleanup

task :mrproper do
  `scons -c`
  `rm -rf build`
  `rm -rf haskell/dist`
  `rm -f #{SRC_DIST}.tar.gz`
  `rm -f #{BIN_DIST}.tar.gz`
end

# EOF