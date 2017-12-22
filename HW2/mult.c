int maxfdp1, stdineof, n;
	fd_set rset;
	char buf[MAXLINE];
	stdineof = 0;
	int stdoutfd = fileno(stdout);
	for ( ; ; ) {
		FD_ZERO(&rset);
		if (stdineof == 0)
			FD_SET(fileno(fp), &rset);
		FD_SET(sockfd, &rset);
		maxfdp1 = max(fileno(fp), sockfd) + 1;
		myselect(maxfdp1, &rset, NULL, NULL, NULL);
		if (FD_ISSET(sockfd, &rset)) { /* socket is readable */
			if ( (n = myread(sockfd, buf, MAXLINE)) == 0) {
				if (stdineof == 1)
					return; /* normal termination */
				else
					show_err_sys("Server terminated prematurely");
			}
			n = write(stdoutfd, buf, n);
		}
		if (FD_ISSET(fileno(fp), &rset)) { /* input is readable */
			if ( (n = myread(fileno(fp), buf, MAXLINE)) == 0) {
				stdineof = 1;
				Shutdown(sockfd, SHUT_WR); /* send FIN */
				FD_CLR(fileno(fp), &rset);
				continue;
			}
			n = write(sockfd, buf, n);
		}
	
	
	docker run -d --net hadoop-net --name slave01 --hostname slave01 cloudsuite/data-analytics slave