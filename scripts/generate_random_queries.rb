#!/usr/bin/env ruby

# Generate a random set of source & destinations pairs to test the monetdb shortest path operators
# The output file should be readable by the monetdb operator graph.loadq
# Usage: ./generate_random_queries.rb -n <NUM_VALUES> -m <MAX_VERTEX_ID> <OUTPUT_FILE>

require "optparse"

# User arguments
$opt_num_values; # the number of values to write
$opt_max_node_id; # the max vertex id, it assumes vertices are in the range [0, max_node_id -1]
$opt_output; # the output file to write

def ctrl()  
  randomgen = Random.new()
  File.open($opt_output, "w"){ |f|
    f.write("# This file contains #{$opt_num_values} values, with max vertex id: #{$opt_max_node_id}\n")
    f.write("# Generated on: #{Time.new().strftime('%H:%M:%S %d-%m-%Y')}\n\n")
    
    $opt_num_values.times() do
      src = randomgen.rand($opt_max_node_id);
      dest = ($opt_max_node_id == 1) ? 0 : randomgen.rand($opt_max_node_id -1);
      # avoid queries with the same value for the source and destination
      dest += 1 if (dest >= src)
      f.puts("#{src} #{dest}");
    end
  }
end

# Parse command line arguments, store the values in the global variables prefixed with opt_
def parse_args(argv)
  program_name = $0;
  usage = "Usage: #{program_name} -n <num_values> -m <max_node_id> <output>"
  
  parser = OptionParser.new{ |opts|
    opts.banner = usage;
    opts.program_name = program_name;
    

    opts.on("-h", "--help", "Shows this help screen"){ 
      puts opts;
      exit; 
    }
    opts.on("-m", "--max_node_id VALUE", Integer, "Set the max vertex id"){ |value|\
      if(value <= 0)
        $stderr.print("Invalid value for --max_node_id: #{value}. It must be a positive number");
        exit 1;
      end
      $opt_max_node_id = value;
    }
    opts.on("-n", "--num_values VALUE", Integer, "Set the number of values to produce"){ |value|
      if (value <= 0)
        $stderr.print("Invalid value for --num_values: #{value}. It must be a positive number.");
        exit 1;
      end
      $opt_num_values = value;
    }
  } # end parser ctor block
  
  if (argv.length == 0) # no arguments given, just show the help
    puts parser
    exit
  end
  
  parser.parse!(argv);
  
  # get the output file
  case(argv.length)
  when 0 # not specified
    $stderr.print("ERROR: Missing argument <output>\n" + usage + "\n");
    exit 1;
  when 1 # ok 
    $opt_output = argv[0];
  else # too many args
    $stderr.print("ERROR: Too many free arguments: " + argv.join(" ") + "\n" + usage + "\n");
    exit 1;
  end
  
  # check whether the arguments num_values and max_node_id has been given
  if($opt_max_node_id.nil?)
    $stderr.print("ERROR: Missing mandatory option --max_node_id <value>\n" + usage + "\n");
    exit 1;
  elsif($opt_num_values.nil?)
    $stderr.print("ERROR: Missing mandatory option --num_values <value>\n" + usage + "\n");
    exit 1;
  end
  
  # check whether to overwrite the output file
  if FileTest.exist?($opt_output)
    if !FileTest.file?($opt_output) 
      $stderr.print("ERROR: The file `#{$opt_output}' already exists but it is not a regular file and cannot be overwritten");
      exit 1;
    end
    
    loop { # while(true) { ... } 
      print("The file `#{$opt_output}' already exists. Do you want to overwrite it? (Y/N): ");
      value = $stdin.readline().chomp();
      case (value)
      when /^(y|yes|yeah)$/i
        break;
      when /^(n|no)$/i
        puts("Aborted by the user!");
        exit;
      else
        puts("ERROR: Invalid value: `#{value}'");
      end
    }
  end # end if, overwrite existing file
  
rescue OptionParser::ParseError => e # base exception thrown by OptionParser
  $stderr.print("ERROR: " + e.message().capitalize() + "\n" + usage + "\n")
  exit 1;
end

def main(argv)
  # parse command line arguments, given by the user
  parse_args(argv)
  # jump to the controller logic
  ctrl()
end

main(ARGV) if __FILE__ == $0;