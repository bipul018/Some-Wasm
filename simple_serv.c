//This code starts a simple html server
#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <math.h>
#ifdef UNICODE
#undef UNICODE
#endif
#ifdef _UNICODE
#undef _UNICODE
#endif
#define _WIN32_LEAN_AND_MEAN
#include <winsock2.h>
#include <ws2tcpip.h>
#include <Windows.h>
#include <inttypes.h>
#include <stdbool.h>
#include <assert.h>

#define nullptr ((void*)0)
#define free_and_null(func, val)			\
  do{ if(val)func(val); val = NULL;} while(0)

#define jmp_on_err(err_cond, jmp_label, ...)	\
  do{if(err_cond){ printf(__VA_ARGS__); goto jmp_label;}}while(0)

typedef struct StringView StringView;
struct StringView {
  size_t len;
  char* base;
};

#define PriSV "%.*s"
#define SVPargs(sv) (int)((sv).len), (sv).base


bool strvieweq(StringView a,StringView b){
  if(a.len > b.len){
    const StringView tmp = a;
    a = b;
    b = tmp;
  }
  
  for(size_t i = 0; i < a.len; ++i){
    if(a.base[i] != b.base[i])
      return false;
    if(!a.base[i] && !b.base[i])
      return true;
  }
  return (a.len == b.len) || (b.base[a.len] == 0);
}

StringView view_cstr(char* str){
  return (StringView){.base = str, .len = strlen(str)};
}


//Any one index -ve means stringview end
//Also any invalid one ersults ni a 0 length
StringView view_substr(StringView sv, int start, int end){
  if(start < 0)
    start = 0;
  if(end < 0)
    end = sv.len - 1;
  int sublen = end - start;
  if(start >= ((int)sv.len - 1)){
    sv.len = 0;
    return sv;
  }

  sv.base += start;
  sv.len -= start;
  if((int)sv.len > sublen)
    sv.len = sublen;
  return sv;
}


//Returns str's len if none found
size_t find_substr(StringView str, StringView substr){

  for(  int inx1 = 0 ;inx1 < (int)str.len - (int)substr.len; ++inx1){

    if(strvieweq(substr, view_substr(str, (int)inx1, (int)inx1 + substr.len))){
      return inx1;
    }
    
  }
  return str.len;
}

typedef uint8_t u8;
typedef uint32_t u32;
typedef int32_t s32;
typedef uint16_t u16;

//Just stichehes content and header of for http 
StringView construct_response(StringView header, StringView content){

  //final len is length of final html file
  //Now we need that for header
  const char* content_len_head = "Content-Length: \r\n\r\n";
  size_t header_len = header.len + strlen(content_len_head) +
    floor(log10(content.len)) + 1;

  StringView new_str = {
     .len = 0,
    .base = malloc(header_len + content.len)
  };
  if(nullptr == new_str.base)
    return new_str;



  memcpy(new_str.base, header.base, header.len);
  new_str.len = header.len;
  
  content_len_head = "Content-Length: ";
  memcpy(new_str.base + new_str.len, content_len_head, strlen(content_len_head));
  new_str.len += strlen(content_len_head);

  sprintf_s(new_str.base + new_str.len, floor(log10(content.len)) + 2, "%d", (int)content.len);
  new_str.len += floor(log10(content.len)) + 1;

  content_len_head = "\r\n\r\n";
  memcpy(new_str.base + new_str.len, content_len_head, strlen(content_len_head));
  new_str.len += strlen(content_len_head);

  memcpy(new_str.base + new_str.len, content.base, content.len);
  new_str.len += content.len;
  
  assert(new_str.len == (header_len + content.len));
  //printf("%d\n", (int)new_str.len);
  return new_str;
  
}

int main(int argc, char* argv[]){

  //Given a port number in argument, then a number of files,
  //This starts a server that simply serves those files

  if(argc < 4){
    printf("Usage : program port root-file-content-type root-file-to-serve [remaining-file-content-type remaining-file-to-serve ]\n");
    return 1;
  }

  //Test if string 1 is pure number type
  int len = strlen(argv[1]);
  for(int i = 0; i < len; ++i){
    if((argv[1][i] < '0') || (argv[1][i] > '9')){
      printf("First argument must be numeric for specifying port\n");
      return 2;
    }
  }

  if((argc %2) != 0){
    printf("Warning, each file must be accompanied by content type to send in header, so total cmd arguments must be even, skipping last argument\n");
  }

  //Construct http response and request header for each argument
  StringView* requests = nullptr;
  StringView* responses = nullptr;
  int serve_count = (argc - 2)/2 + 1;
  requests = malloc(serve_count * sizeof * requests);
  responses = malloc(serve_count * sizeof * requests);

  StringView no_content_header = view_cstr("HTTP/1.1 204 No Content\r\n\r\n");
  if((nullptr == requests) || (nullptr == responses)){
    printf("Memory allocation error\n");
  }

  //append a space at end of request header
  const StringView sample_req_header = view_cstr("GET /");
  //append \r\n after
  const StringView sample_res_header = view_cstr("HTTP/1.1 200 OK \r\nConnection: close\r\nContent-Type: ");

  int max_len_req = 0;
  for(int i = 0; i < serve_count; ++i){
    StringView name = {0};
    StringView type = {0};

    if(i < (serve_count - 1)){
      name = view_cstr(argv[3 + i*2]);
      if(name.len > max_len_req)
	max_len_req = name.len;
    
      type = view_cstr(argv[2 + i*2]);
    }
    else{
      //A pseudo path for root
      name = view_cstr("");
      type = view_cstr(argv[2]);
    }

    requests[i].len = name.len + sample_req_header.len + 1;
    responses[i].len = type.len + sample_res_header.len + 2;
    
    requests[i].base = malloc(requests[i].len);
    responses[i].base = malloc(responses[i].len);

    if((nullptr == requests[i].base) ||
       (nullptr == responses[i].base)){
      printf("Memory allocation error\n");
      return 2;
    }
    
    memcpy(requests[i].base, sample_req_header.base, sample_req_header.len);
    memcpy(requests[i].base + sample_req_header.len,
	   name.base, name.len);
    memcpy(requests[i].base + requests[i].len - 1, " ", 1);

    memcpy(responses[i].base, sample_res_header.base, sample_res_header.len);
    memcpy(responses[i].base + sample_res_header.len,
	   type.base, type.len);
    memcpy(responses[i].base + responses[i].len - 2, "\r\n", 2);
  }

  StringView recv_buffer = {
    .base = malloc(max_len_req*2) //I think some whitespaces are allowd 
  };
  
  if(nullptr == recv_buffer.base){
    printf("Memory allocation error\n");
    return 2;
  }
  recv_buffer.len = max_len_req * 2;
  
  WSADATA wsa_data;
  int res_code;
  
  res_code = WSAStartup(MAKEWORD(2, 2), &wsa_data);
  jmp_on_err(res_code != 0,
	     wsastart,
	     "WSAStartup failed with error %d\n", res_code);

  const char* port = argv[1];

  struct addrinfo conn_hints = {
    //In tutirual INET was specified here
    .ai_family = AF_INET,
    .ai_socktype = SOCK_STREAM,
    .ai_protocol = IPPROTO_TCP,
    .ai_flags = AI_PASSIVE
  };
  struct addrinfo* possible_addrs = NULL;
  res_code = getaddrinfo(NULL, port, &conn_hints, &possible_addrs);
  jmp_on_err(res_code != 0, getaddrinfo,
	     "getaddrinfo for finding address for port %s failed with code %d\n",
	     port, res_code);
  int count_of_addrs = 0;
  for(struct addrinfo* ptr = possible_addrs; ptr != NULL; ptr = ptr->ai_next){
    count_of_addrs++;
  }
  printf("The possible number of addresses are %d\n", count_of_addrs);

  //One socket for listening to connections
  //Later when a connection happens, we create a temporary socket for the client connection
  SOCKET listening_socket = INVALID_SOCKET;

  listening_socket = socket(possible_addrs->ai_family, possible_addrs->ai_socktype, possible_addrs->ai_protocol);
  jmp_on_err(INVALID_SOCKET == listening_socket, listen_socket,
	     "Couldnot open listening socket, error code :%d\n", (int)WSAGetLastError());
  res_code = bind(listening_socket, possible_addrs->ai_addr, possible_addrs->ai_addrlen);
  jmp_on_err(SOCKET_ERROR == res_code, bind_fail,
	     "Failure during bind() with code %d\n", (int)WSAGetLastError());
  free_and_null(freeaddrinfo, possible_addrs);

  res_code = listen(listening_socket, SOMAXCONN);
  jmp_on_err(SOCKET_ERROR == res_code, bind_fail,
	     "Listening operation for client failed with error %d\n", (int)WSAGetLastError());

  while(true){
  
    SOCKET client_socket = accept(listening_socket, NULL, NULL);
    jmp_on_err(INVALID_SOCKET == client_socket, loop_skip,
	       "Acceptance of a client socket failed with error code : %d\n", WSAGetLastError());

    memset(recv_buffer.base, 0, recv_buffer.len);
    res_code = recv(client_socket, recv_buffer.base, recv_buffer.len, 0);
    
    if(res_code > 0){
      //Compare and send response
      StringView to_send = {0};
      for(int i = 0; i < serve_count; ++i){
	size_t substr_pos = find_substr(recv_buffer, requests[i]);
	if(substr_pos != recv_buffer.len){
	  //Construct response
	  StringView file_read = {0};
	  FILE* file = nullptr;
	  //Handle root case separately
	  if(i < (serve_count - 1)){
	    file = fopen(argv[3 + i * 2], "rb");
	  }
	  else{
	    file = fopen(argv[3], "rb");
	  }
	  if(nullptr == file)
	    break;
	  fseek(file, 0, SEEK_END);
	  file_read.len = ftell(file);
	  fseek(file, 0, SEEK_SET);
	  file_read.base = malloc(file_read.len);
	  if(nullptr == file_read.base){
	    fclose(file);
	    break;
	  }
	  fread(file_read.base, 1, file_read.len, file);
	  fclose(file);
	  
	  to_send = construct_response(responses[i], file_read);
	  free(file_read.base);
	  break;
	}
      }
      if(nullptr == to_send.base){
	res_code = send(client_socket, no_content_header.base,
			no_content_header.len, 0);
	if(SOCKET_ERROR == res_code){
	  printf("Some error occured in sending the client somethings : %d\n",
		 WSAGetLastError());
	}
      }
      else{
	res_code = send(client_socket, to_send.base,to_send.len, 0);
	if(SOCKET_ERROR == res_code){
	  printf("Some error occured in sending the client somethings : %d\n",
		 WSAGetLastError());
	}
	free(to_send.base);
      }
    }
    else{
      printf("Some error occured in receiving client request, code : %d\n",
	     WSAGetLastError());
    }
    
  loop_skip:
    continue;
  }

  

 bind_fail:
  closesocket(listening_socket);
 listen_socket:
  free_and_null(freeaddrinfo, possible_addrs);
 getaddrinfo:
  WSACleanup();
 wsastart:
  return 0;
}
