# include <unistd.h>
# include <string.h>
# include <stdio.h>
# include <stdlib.h>
# include <sys/types.h>
# include <sys/wait.h>

int		g_status = 0;
int		n_pid = 0;

typedef struct	s_ast
{
	char		**arg;
	char		type;
	struct s_ast	*next;
	struct s_ast	*prev;
}				t_ast;

void	ft_putstr_fd(char *str, int fd)
{
	write(fd, str, strlen(str));
}

void	print_matc(char **matc)
{
	if (!matc)
		return ;
	while (*matc)
	{
		printf("| %s | ", *matc);
		matc++;
	}
	printf("\n");
}

void	print_ast(t_ast *ast)
{
	t_ast	*next;

	next = ast;
	while (next)
	{
		printf("node : \ntype : %c\n", ast->type);
		print_matc(next->arg);
		ast = next;
		next = ast->next;
	}
}

void	free_ast(t_ast *ast)
{
	t_ast	*tmp;
	int		i;

	if (!ast)
		return ;
	while (ast->prev)
		ast = ast->prev;
	while (ast)
	{
		tmp = ast->next;
		i = 0;
		while (ast->arg[i])
		{
			free(ast->arg[i]);
			i++;
		}
		free(ast->arg);
		free(ast);
		ast = tmp;
	}
}

void	free_fds(int **fds)
{
	int		i;

	i = 0;
	while (fds[i])
	{
		free(fds[i]);
		i++;
	}
	free(fds);
}

size_t	ft_strlen(char *s)
{
	size_t	len;

	len = 0;
	if (!s)
		return (0);
	while (s[len])
		len++;
	return (len);
}

char	*ft_strdup(char *src)
{
	char	*dest;
	int		i;

	i = 0;
	dest = malloc(ft_strlen(src) + 1);
	if (!dest)
		return (0);
	while (src[i])
	{
		dest[i] = src[i];
		i++;
	}
	dest[i] = 0;
	return (dest);
}

t_ast	*ft_astlast(t_ast *ast)
{
	t_ast	*next;

	if (ast == NULL)
		return (0);
	next = ast->next;
	while (next)
	{
		ast = next;
		next = ast->next;
	}
	return (ast);
}

void	ft_astadd_back(t_ast **astk, t_ast *new)
{
	t_ast	*last;

	last = ft_astlast(*astk);
	if (last)
	{
		last->next = new;
		new->prev = last;
	}
	else
	{
		*astk = new;
		new->prev = NULL;
	}
}

void	ft_error_stay(char *msg, char *arg, t_ast *ast)
{
	write(2, msg, ft_strlen(msg));
	if (arg)
		write(2, msg, ft_strlen(arg));
	write(2, "\n", 1);
	free_ast(ast);
}

void	ft_error(char *msg, char *arg, t_ast *ast)
{
	write(2, msg, ft_strlen(msg));
	if (arg)
		write(2, msg, ft_strlen(arg));
	write(2, "\n", 1);
	free_ast(ast);
	exit(1);
}

int		get_n_args(int n_arg, int ac, char **av)
{
	int	n;

	n = 0;
	while (n_arg + n < ac && strcmp(av[n_arg + n], ";")
			&& strcmp(av[n_arg + n], "|"))
		n++;
	return (n);
}

t_ast	*build_one_node(int *n_arg, int ac, char **av)
{
	int		i;
	t_ast	*ast;

	if (*n_arg >= ac)
		return (0);
	ast = malloc(sizeof(t_ast));
	if (!ast)
		ft_error("error: fatal", NULL, ast);
	i = 0;
	ast->arg = malloc(sizeof(char *) * (get_n_args(*n_arg, ac, av) + 1));
	if (!ast->arg)
		ft_error("error: fatal", NULL, ast);
	while (*n_arg < ac && strcmp(av[*n_arg], ";")
			&& strcmp(av[*n_arg], "|"))
	{
		ast->arg[i] = ft_strdup(av[*n_arg]);
		if (!ast->arg[i])
			ft_error("error: fatal", NULL, ast);
		i++;
		*n_arg += 1;
	}
	if (*n_arg < ac && !strcmp(av[*n_arg], "|")) 
		ast->type = 'c';
	else if (*n_arg < ac)
		ast->type = 'e';
	else
		ast->type = 'd';
	ast->arg[i] = 0;
	ast->next = NULL;
	*n_arg += 1;
	n_pid++;
	return (ast);
}

t_ast	*build_ast(int ac, char **av)
{
	t_ast	*ast;
	int		n_arg;

	n_arg = 1;
	ast = build_one_node(&n_arg, ac, av);
	ast->prev = NULL;
	while (n_arg < ac)
		ft_astadd_back(&ast, build_one_node(&n_arg, ac, av));
	return (ast);
}

int		**init_fds(int n_cmd)
{
	int		**fds;
	int		c;

	fds = malloc(sizeof(int *) * (n_cmd + 1));
	if (!fds)
		return (0);
	c = 0;
	while (c < n_cmd)
	{
		fds[c] = malloc(sizeof(int) * 2);
		if (!fds[c] || pipe(fds[c]) == -1)
		{
			while (--c >= 0)
				free(fds[c]);
			free(fds);
			return (0);
		}
		c++;
	}
	fds[c] = 0;
	return (fds);
}

int	ft_arglen(char **arg)
{
	int	i;

	i = 0;
	while (arg[i])
		i++;
	return (i);
}

void	ft_cd(t_ast *ast)
{
	if (ft_arglen(ast->arg) > 2)
		ft_error_stay("error: cd: bad arguments", NULL, ast);
	else if (chdir(ast->arg[1]) == -1)
		ft_error_stay("error: cd: cannot change directory\n", ast->arg[1], ast);
}

void	close_for_zero(int **fds)
{
	int		i;

	i = 1;
	close(fds[0][0]);
	while (i < n_pid)
	{
		close(fds[i][0]);
		close(fds[i][1]);
		i++;
	}
}

void	close_for_n(int **fds, int i)
{
	int		j;

	j = 0;
	while (j < n_pid)
	{
		if (j == i - 1)
			close(fds[j][1]);
		else if (j == i)
			close(fds[j][0]);
		else
		{
			close(fds[j][0]);
			close(fds[j][1]);
		}
		j++;
	}
}

void	manage_fd(int **fds, int i, t_ast  *ast)
{
	if (i == 0)
	{
		close_for_zero(fds);
		if (n_pid > 1 && ast->type != 'e')
			dup2(fds[i][1], STDOUT_FILENO);
	}
	else
	{
		close_for_n(fds, i);
		if (ast->prev->type != 'e')
			dup2(fds[i - 1][0], STDIN_FILENO);
		close(fds[i - 1][0]);
		if (ast->type != 'e')
			dup2(fds[i][1], STDOUT_FILENO);
	}
}

void	ft_child(t_ast *ast, int **fds, char **envp, int i, pid_t *child_pid)
{
	manage_fd(fds, i, ast);
	if (!strcmp(ast->arg[0], "cd"))
		ft_cd(ast);
	else
	{
		execve(ast->arg[0], ast->arg, envp);
		write(2, "error: cannot execute ", 22);
		write(2, ast->arg[0], strlen(ast->arg[0]));
		write(2, "\n", 1);
	}
	close(fds[i][1]);
	if (child_pid[i] != 42)
	{
		close(1);
		close(0);
		free_ast(ast);
		free(child_pid);
		free_fds(fds);
		exit(g_status);
	}
}

void	wait_pid(pid_t *child_pid, int i)
{
	int		j;

	j = 0;
	while (j <= i)
	{
		waitpid(child_pid[j], &g_status, 0);
		if (WIFEXITED(g_status))
			g_status = WEXITSTATUS(g_status);
		j++;
	}
}

void	close_fds_in_parent(int **fds, int i, int config)
{
	if (config == 1)
	{
		if (i > 0)
			close(fds[i - 1][0]);
		close(fds[i][1]);
	}
	else
	{
		close(fds[i][0]);
		if (i > 0)
			close(fds[i - 1][0]);
	}
}

void	ft_microshell(t_ast *ast, pid_t *child_pid, int **fds, char **envp)
{
	t_ast	*next;
	int	i;

	next = ast;
	i = -1;
	while (++i < n_pid)
	{
		if ((!next->prev || next->prev->type == 'e')
				&& (!next->next || next->next->type == 'e')
				&& !strcmp(next->arg[0], "cd"))
			child_pid[i] = 42;
		else
			child_pid[i] = fork();
		if (child_pid[i] == -1)
		{
			free(child_pid);
			free(fds);
			ft_error("error: fatal", NULL, ast);
		}
		else if (child_pid[i] == 0 || child_pid[i] == 42)
			ft_child(next, fds, envp, i, child_pid);
		close_fds_in_parent(fds, i, 1);
		if (next->type == 'e')
			wait_pid(child_pid, i);
		next = next->next;
	}
	close_fds_in_parent(fds, n_pid - 1, 0);
	wait_pid(child_pid, n_pid - 1);
}

int	main(int ac, char **av, char **envp)
{
	t_ast	*ast;
	pid_t	*child_pid;
	int		**fds;

	ast = build_ast(ac, av);
	if (!n_pid)
		return (0);
	child_pid = malloc(sizeof(pid_t) * n_pid);
	if (!child_pid)
			ft_error("error: fatal", NULL, ast);
	fds = init_fds(n_pid);
	if (!fds)
	{
			free(child_pid);
			ft_error("error: fatal", NULL, ast);
	}
	ft_microshell(ast, child_pid, fds, envp);
	free_ast(ast);
	free_fds(fds);
	free(child_pid);
	return (g_status);
}
