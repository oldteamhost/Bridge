/*
 * bridge.cc
 * By oldteam & lomaster
 * License CC-BY-NC-SA 4.0
 * Thanks to ......2007
 *   Сделано от души 2023.
*/

/* C */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* libs */
#include <curl/curl.h>

/* C++ */
#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <iomanip>
#include <chrono>
#include <sstream>

#define VERSION "0.1"

#undef no_argument
#define no_argument 0
#undef required_argument
#define required_argument 1
#undef optional_argument
#define optional_argument 2
struct option
{
  const char *name;
  int has_arg;
  int *flag;
  int val;
};
extern int
getopt_long_only(int argc, char * argv[], const char *shortopts, const struct option *longopts, int *longind);
extern int
_getopt_internal(int argc, char * argv[], const char *shortopts, const struct option *longopts, int *longind,int long_only);
extern int
getopt_reset(void);
extern int optind, opterr, optopt;
extern char *optarg;
int optind=1, opterr=1, optopt=0; char *optarg=0;

/* bridge */
void usage();
/* exit and print message */
void quitting();
/* split_string_string */
std::vector<std::string>
split_string_string(const std::string& str, char delimiter);
/* read file by line in vector */
std::vector<std::string>
write_file(const std::string& filename);
/* encode base64 */
const std::string base64_chars =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
    "abcdefghijklmnopqrstuvwxyz"
    "0123456789+/";
std::string base64_encode(const unsigned char* input, size_t length);
/* decode string url */
std::vector<std::string>
decode_string_dav(const std::string& input);

/* network bridge */
size_t read_callback(void* buffer, size_t size, size_t nmemb, void* userp);
size_t write_data(void* ptr, size_t size, size_t nmemb, FILE* stream);
int drop_file(const std::string& url, const std::string& filename, bool tor, bool verbose);
int get_file(const std::string& url, const std::string& filename, bool tor, bool verbose);

const struct option longopts[] =
{
    {"help", no_argument, 0, 'h'},
    {"verbose", no_argument, 0, 'V'},
    {"version", no_argument, 0, 'v'},
    {"files", required_argument, 0, 'f'},
    {"export", no_argument, 0, 'e'},
    {"import", no_argument, 0, 'i'},
    {"consensuses", required_argument, 0, 'c'}
};

const char* shortopts = "hvf:c:ieV";
std::vector<std::string> consensuses;
std::vector<std::string> files;

bool _import = false;
bool _export = false;
bool verbose = false;

int
main(int argc, char** argv)
{
    if (argc <= 1)
    {
        usage();
        quitting();
    }

    int rez, option_index = 0;
    while ((rez = getopt_long_only(argc, argv, shortopts, longopts, &option_index)) != -1)
    {
        switch (rez)
        {
		  case 'h':
			 usage();
             quitting();
			 break;
		  case 'v':
             printf("Created by oldteam & lomaster: %s\n", VERSION);
             quitting();
			 break;
		  case 'e':
             _export = true;
			 break;
		  case 'V':
             verbose = true;
			 break;
		  case 'i':
             _import = true;
			 break;
		  case 'f':
             files = split_string_string(optarg, ',');
			 break;
		  case 'c':
             consensuses = write_file(optarg);
			 break;
        }
    }

    if (optind < argc)
    {
       consensuses = split_string_string(argv[optind], ',');
    }

    time_t now = time(NULL);
    struct tm *t = localtime(&now); char datetime[20];
    strftime(datetime, sizeof(datetime), "%Y-%m-%d %H:%M:%S", t);
    printf("Running Bridge %s at %s\n", VERSION, datetime);
    printf("Loaded %lu conseunces, they`ve been run.\n", consensuses.size());
    if (consensuses.size() <= 0)
    {
        printf("You must specify at least one consensus!");
        quitting();
    }

    std::string action;
    if (_import)
    {
        action = "import";
    }
    else if (_export)
    {
        action = "export";
    }

    for (const auto& f : files){
        printf("\nA process with a file \"%s\" has been started.\n", f.c_str());
        for (const auto& c : consensuses){
            printf("* Trying %s consensus: %s\n", action.c_str(), c.c_str());
            if (_import)
            {
                drop_file(c, f, true, verbose);
            }
            if (_export)
            {
                get_file(c, f, true, verbose);
            }
        }
    }
    quitting();
    return 0;
}

std::vector<std::string>
decode_string_dav(const std::string& input)
{
    std::vector<std::string> result;
    std::istringstream ss(input);
    std::string token;
    std::getline(ss, token, '/');
    std::getline(ss, token, '/');
    std::getline(ss, token, '/');
    std::string domain = token;

    std::string lastToken;
    while (std::getline(ss, token, '/')) {
        lastToken = token;
    }

    result.push_back(domain);
    result.push_back(lastToken);

    return result;
}

size_t read_callback(void* buffer, size_t size, size_t nmemb, void* userp)
{
    std::ifstream* fileStream = static_cast<std::ifstream*>(userp);
    if (fileStream->good())
    {
        fileStream->read(static_cast<char*>(buffer), size * nmemb);
        return static_cast<size_t>(fileStream->gcount());
    }

    return 0;
}

int drop_file(const std::string& url, const std::string& filename, bool tor, bool verbose)
{
    auto start_time = std::chrono::high_resolution_clock::now();
    curl_global_init(CURL_GLOBAL_DEFAULT);
    CURL* curl = curl_easy_init();
    if (!curl)
    {
        puts("Curl failed to initialize, check the internet!");
        return 1;
    }

    size_t size_in_bytes;
    std::ifstream file_stream(filename);
    if (!file_stream.is_open())
    {
        printf("File %s not found!\n", filename.c_str());
        curl_easy_cleanup(curl);
        return 1;
    }
    else
    {
        file_stream.seekg(0, std::ios::end);
        std::streampos file_size = file_stream.tellg();
        size_in_bytes = static_cast<size_t>(file_size);
        std::cout << "* File size: " << size_in_bytes << " bytes" << std::endl;
        file_stream.seekg(0, std::ios::beg);
    }

    if (tor)
    {
        curl_easy_setopt(curl, CURLOPT_PROXY, "socks5h://localhost:9050");
    }

    std::vector<std::string> parts = decode_string_dav(url);
    std::string result_url = parts[0] + "/public.php/webdav/" + filename;
    std::string auth_pre = parts[1] + ":";
    std::string auth = base64_encode(reinterpret_cast<const unsigned char*>(auth_pre.c_str()), auth_pre.length());
    std::string auth_result = "Authorization: Basic " + auth;
    std::cout << "* " << auth_result << std::endl;

    if (verbose)
    {
        curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
    }

    curl_easy_setopt(curl, CURLOPT_URL, result_url.c_str());
    curl_easy_setopt(curl, CURLOPT_UPLOAD, 1L);
    curl_easy_setopt(curl, CURLOPT_READFUNCTION, read_callback);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, curl_slist_append(NULL, auth_result.c_str()));
    curl_easy_setopt(curl, CURLOPT_READDATA, &file_stream);
    curl_easy_setopt(curl, CURLOPT_INFILESIZE_LARGE, static_cast<curl_off_t>(size_in_bytes));

    CURLcode res = curl_easy_perform(curl);
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
    double seconds = duration.count() / 1000000.0;
    if (res != CURLE_OK) {
        std::cerr << "curl_easy_perform() failed: " << curl_easy_strerror(res) << std::endl;
        long response_code;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);
        std::cout << "HTTP Response Code: " << response_code << std::endl;
    } else {
        std::cout << std::fixed << std::setprecision(2);
        std::cout << "File uploaded successfully at " << seconds << "s"  << std::endl;
    }

    curl_easy_cleanup(curl);
    curl_global_cleanup();
    file_stream.close();
    return 0;
}


size_t write_data(void* ptr, size_t size, size_t nmemb, FILE* stream)
{
	size_t written = fwrite(ptr, size, nmemb, stream);
	return written;
}

long long int download_size = 0;

size_t header_callback(void *ptr, size_t size, size_t nmemb, void *userdata)
{
    size_t total_size = size * nmemb;
    std::string header(reinterpret_cast<char*>(ptr), total_size);

    std::string content_length_header = "Content-Length: ";
    size_t content_length_pos = header.find(content_length_header);
    if (content_length_pos != std::string::npos) {
        std::string content_length_value = header.substr(content_length_pos + content_length_header.length());
        download_size = std::stoll(content_length_value);
    }

    return total_size;
}

int get_file(const std::string& url, const std::string& filename, bool tor, bool verbose)
{
    auto start_time = std::chrono::high_resolution_clock::now();
    std::string result_url = url + "/download?path=/&files=" + filename;
    curl_global_init(CURL_GLOBAL_DEFAULT);
    CURL* curl = curl_easy_init();
    if (!curl)
    {
        std::cerr << "Curl failed to initialize, check the internet!" << std::endl;
        return 1;
    }
    
    FILE* fp = fopen(filename.c_str(), "wb");
    if (!fp)
    {
        std::cerr << "Failed to open file for writing: " << filename << std::endl;
        curl_easy_cleanup(curl);
        curl_global_cleanup();
        return 1;
    }
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
    curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, header_callback);
    
    if (tor)
    {
        curl_easy_setopt(curl, CURLOPT_PROXY, "socks5h://localhost:9050");
    }
    curl_easy_setopt(curl, CURLOPT_URL, result_url.c_str());

    if (verbose)
    {
        curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
    }

    CURLcode res = curl_easy_perform(curl);
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
    double seconds = duration.count() / 1000000.0;
    if (res != CURLE_OK)
    {
        std::cerr << "Curl failed: " << curl_easy_strerror(res) << std::endl;
        fclose(fp);
        curl_easy_cleanup(curl);
        curl_global_cleanup();
        return 1;
    }
    std::cout << "* File size: " << download_size << " bytes" << std::endl;
    std::cout << std::fixed << std::setprecision(2);
    std::cout << "The file has been successfully downloaded at " << seconds << "s" << std::endl;
    download_size = 0;
    
    fclose(fp);
    curl_easy_cleanup(curl);
    curl_global_cleanup();

    return 0;
}

std::string base64_encode(const unsigned char* input, size_t length) {
    std::string encoded_data;
    int i = 0;
    int j = 0;
    unsigned char char_array_3[3];
    unsigned char char_array_4[4];

    while (length--) {
        char_array_3[i++] = *(input++);
        if (i == 3) {
            char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
            char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
            char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
            char_array_4[3] = char_array_3[2] & 0x3f;

            for (i = 0; i < 4; i++) {
                encoded_data += base64_chars[char_array_4[i]];
            }
            i = 0;
        }
    }

    if (i) {
        for (j = i; j < 3; j++) {
            char_array_3[j] = '\0';
        }

        char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
        char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
        char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);

        for (j = 0; j <= i; j++) {
            encoded_data += base64_chars[char_array_4[j]];
        }

        while (i++ < 3) {
            encoded_data += '=';
        }
    }

    return encoded_data;
}

void quitting()
{
    printf("\nSUBVERSION!\n");
    exit(0);
}

std::vector<std::string>
split_string_string(const std::string& str, char delimiter)
{
    std::vector<std::string> result;
    size_t pos = 0, found;
    std::string token;
    while ((found = str.find_first_of(delimiter, pos)) != std::string::npos)
	{
        token = str.substr(pos, found - pos);
        result.push_back(token);
        pos = found + 1;
    }
    result.push_back(str.substr(pos));
    return result;
}

std::vector<std::string>
write_file(const std::string& filename)
{
    std::vector<std::string> lines;
    std::string line;

    std::ifstream file(filename);
	if (file.fail()){return {"-1"};}
    while (std::getline(file, line))
    {lines.push_back(line);}
    return lines;
}

void usage()
{
    printf("usage: ./bridge [consensus 1,2,3...] [flags]\n\n");

    puts("arguments program:");
    puts("  -h, -help         Show this help message.");
    puts("  -V, -verbose      Display verbose info.");
    puts("  -v, -version      Show this help message.");

    puts("\narguments target:");
    puts("  -f, -files        Set list of files.");
    puts("  -c, -consensuses  Specify the file with the conseunces.");

    puts("\narguments action:");
    puts("  -i, -import       Transfer the file over the bridge.");
    puts("  -e, -export       Enter the file on the bridge.");
}

static int 
_getopt(int argc, char * argv[], const char *opts)
{
  static int charind=0;
  const char *s;
  char mode, colon_mode;
  int off = 0, opt = -1;

  if(getenv("POSIXLY_CORRECT")) colon_mode = mode = '+';
  else {
    if((colon_mode = *opts) == ':') off ++;
    if(((mode = opts[off]) == '+') || (mode == '-')) {
      off++;
      if((colon_mode != ':') && ((colon_mode = opts[off]) == ':'))
        off ++;
    }
  }
  optarg = 0;
  if(charind) {
    optopt = argv[optind][charind];
    for(s=opts+off; *s; s++) if(optopt == *s) {
      charind++;
      if((*(++s) == ':') || ((optopt == 'W') && (*s == ';'))) {
        if(argv[optind][charind]) {
          optarg = &(argv[optind++][charind]);
          charind = 0;
        } else if(*(++s) != ':') {
          charind = 0;
          if(++optind >= argc) {
		  	if(opterr) fprintf(stderr,"This bad, check help_menu.\n");
            opt = (colon_mode == ':') ? ':' : '?';
            goto my_getopt_ok;
          }
          optarg = argv[optind++];
        }
      }
      opt = optopt;
      goto my_getopt_ok;
    }
    if(opterr) fprintf(stderr,
                        "%s: What fuck? -- %c\n",
                        argv[0], optopt);
    opt = '?';
    if(argv[optind][++charind] == '\0') {
      optind++;
      charind = 0;
    }
  my_getopt_ok:
    if(charind && ! argv[optind][charind]) {
      optind++;
      charind = 0;
    }
  } else if((optind >= argc) ||
             ((argv[optind][0] == '-') &&
              (argv[optind][1] == '-') &&
              (argv[optind][2] == '\0'))) {
    optind++;
    opt = -1;
  } else if((argv[optind][0] != '-') ||
             (argv[optind][1] == '\0')) {
    char *tmp;
    int i, j, k;

    if(mode == '+') opt = -1;
    else if(mode == '-') {
      optarg = argv[optind++];
      charind = 0;
      opt = 1;
    } else {
      for(i=j=optind; i<argc; i++) if((argv[i][0] == '-') &&
                                        (argv[i][1] != '\0')) {
        optind=i;
        opt=_getopt(argc, argv, opts);
        while(i > j) {
          tmp=argv[--i];
          for(k=i; k+1<optind; k++) argv[k]=argv[k+1];
          argv[--optind]=tmp;
        }
        break;
      }
      if(i == argc) opt = -1;
    }
  } else {
    charind++;
    opt = _getopt(argc, argv, opts);
  }
  if (optind > argc) optind = argc;
  return opt;
}

int 
_getopt_internal(int argc, char * argv[], const char *shortopts,
                     const struct option *longopts, int *longind,
                     int long_only){
  char mode, colon_mode = *shortopts;
  int shortoff = 0, opt = -1;

  if(getenv("POSIXLY_CORRECT")) colon_mode = mode = '+';
  else {
    if((colon_mode = *shortopts) == ':') shortoff ++;
    if(((mode = shortopts[shortoff]) == '+') || (mode == '-')) {
      shortoff++;
      if((colon_mode != ':') && ((colon_mode = shortopts[shortoff]) == ':'))
        shortoff ++;
    }
  }
  optarg = 0;
  if((optind >= argc) ||
      ((argv[optind][0] == '-') &&
       (argv[optind][1] == '-') &&
       (argv[optind][2] == '\0'))) {
    optind++;
    opt = -1;
  } else if((argv[optind][0] != '-') ||
            (argv[optind][1] == '\0')) {
    char *tmp;
    int i, j, k;

    opt = -1;
    if(mode == '+') return -1;
    else if(mode == '-') {
      optarg = argv[optind++];
      return 1;
    }
    for(i=j=optind; i<argc; i++) if((argv[i][0] == '-') &&
                                    (argv[i][1] != '\0')) {
      optind=i;
      opt=_getopt_internal(argc, argv, shortopts,
                           longopts, longind,
                           long_only);
      while(i > j) {
        tmp=argv[--i];
        for(k=i; k+1<optind; k++)
          argv[k]=argv[k+1];
        argv[--optind]=tmp;
      }
      break;
    }
  } else if((!long_only) && (argv[optind][1] != '-'))
    opt = _getopt(argc, argv, shortopts);
  else {
    int charind, offset;
    int found = 0, ind, hits = 0;

    if(((optopt = argv[optind][1]) != '-') && ! argv[optind][2]) {
      int c;

      ind = shortoff;
      while((c = shortopts[ind++])) {
        if(((shortopts[ind] == ':') ||
            ((c == 'W') && (shortopts[ind] == ';'))) &&
           (shortopts[++ind] == ':'))
          ind ++;
        if(optopt == c) return _getopt(argc, argv, shortopts);
      }
    }
    offset = 2 - (argv[optind][1] != '-');
    for(charind = offset;
        (argv[optind][charind] != '\0') &&
          (argv[optind][charind] != '=');
        charind++);
    for(ind = 0; longopts[ind].name && !hits; ind++)
      if((strlen(longopts[ind].name) == (size_t) (charind - offset)) &&
		  (strncmp(longopts[ind].name,
				argv[optind] + offset, charind - offset) == 0))
		  found = ind, hits++;
	   if(!hits) for(ind = 0; longopts[ind].name; ind++)
	   if(strncmp(longopts[ind].name,
				argv[optind] + offset, charind - offset) == 0)
		  found = ind, hits++;
	   if(hits == 1) {
	   opt = 0;

	   if(argv[optind][charind] == '=') {
		  if(longopts[found].has_arg == 0) {
		  opt = '?';
		  if(opterr) fprintf(stderr,"This bad, check help_menu.\n");
		  } else {
		  optarg = argv[optind] + ++charind;
		  charind = 0;
		  }
	   } else if(longopts[found].has_arg == 1) {
		  if(++optind >= argc) {
		  opt = (colon_mode == ':') ? ':' : '?';
		  if(opterr) fprintf(stderr,"This bad, check help_menu.\n");
		  } else optarg = argv[optind];
	   }
	   if(!opt) {
		  if (longind) *longind = found;
		  if(!longopts[found].flag) opt = longopts[found].val;
		  else *(longopts[found].flag) = longopts[found].val;
	   }
		  optind++;
	   } else if(!hits) {
	   if(offset == 1) opt = _getopt(argc, argv, shortopts);
	   else {
		  opt = '?';
		  if(opterr) fprintf(stderr,
					   "%s: what?`%s'\n",
					   argv[0], argv[optind++]);
	   }
	   } else {
	   opt = '?';
	   if(opterr) fprintf(stderr,
					   "%s: option `%s' what?\n",
					   argv[0], argv[optind++]);
	   }
    }
    if (optind > argc) optind = argc;
    return opt;
}

int 
getopt_long_only(int argc, char * argv[], const char *shortopts,
                const struct option *longopts, int *longind)
{
    return _getopt_internal(argc, argv, shortopts, longopts, longind, 1);
}

int getopt_reset(void)
{
    optind = 1;
    opterr = 1;
    optopt = 0;
    optarg = 0;
    return 0;
}
