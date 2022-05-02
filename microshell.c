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
	char			**arg;
	struct s_ast	*next;
}				t_ast;

void	free_fds(int **fds)
{
	int		i;

	i = 0;
	while (fds[i])
	{
		free(fds[i]);
		i++;
	}
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
		printf("node : \n");
		print_matc(next->arg);
		ast = next;
		next = ast->next;
	}
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
		last->next = new;
	else
		*astk = new;
}

void	free_ast(t_ast *ast)
{
	t_ast	*tmp;
	int		i;

	if (!ast)
		return ;
	while (ast)
	{
		tmp = ast->next;
		i = 0;
		while (ast->arg[i])
		{
			free(ast->arg[i]);
			i++;
		}
		free(ast);
		ast = tmp;
	}
}

void	ft_error(char *msg, t_ast *ast)
{
	write(2, msg, ft_strlen(msg));
	free_ast(ast);
	exit(1);
}

int		get_n_args(int n_arg, int ac, char **av)
{
	int	n;

	n = 0;
	while (n_arg + n < ac && strcmp(av[n_arg + n], "|"))
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
		ft_error("error: fatal\n", ast);
	i = 0;
	ast->arg = malloc(sizeof(char *) * (get_n_args(*n_arg, ac, av) + 1));
	if (!ast->arg)
		ft_error("error: fatal\n", ast);
	while (*n_arg < ac && strcmp(av[*n_arg], "|"))
	{
		ast->arg[i] = ft_strdup(av[*n_arg]);
		if (!ast->arg[i])
			ft_error("error: fatal\n", ast);
		i++;
		*n_arg += 1;
	}
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

void	ft_cd(t_ast *ast, int **fds)
{
	(void)ast;
	(void)fds;
	printf("cd\n");
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

void	manage_fd(int **fds, int i)
{
	if (i == 0)
	{
		close_for_zero(fds);
		if (n_pid > 1)
			dup2(fds[i][1], STDOUT_FILENO);
		close(fds[i][1]);
	}
	else if (i < n_pid - 1)
	{
		close_for_n(fds, i);
		dup2(fds[i][0], STDIN_FILENO);
		dup2(fds[i][1], STDOUT_FILENO);
		close(fds[i][0]);
		close(fds[i][1]);
	}
	else
	{
		close_for_n(fds, i);
		close(fds[i][1]);
		dup2(fds[i][0], STDIN_FILENO);
		close(fds[i][0]);
	}
}

void	ft_child(t_ast *ast, int **fds, char **envp, int i)
{
	manage_fd(fds, i);
	if (!strcmp(ast->arg[0], "cd"))
		ft_cd(ast, fds);
	else if (execve(ast->arg[0], ast->arg, envp) < 0)
		g_status = 1;
	exit(g_status);
}

void	wait_pid(pid_t *child_pid)
{
	int		i;

	i = 0;
	while (i < n_pid - 1)
	{
		waitpid(child_pid[i], &g_status, 0);
		if (WIFEXITED(g_status))
			g_status = WEXITSTATUS(g_status);
		i++;
	}
}

void	close_fds_in_parent(int **fds, int i, int config)
{
	if (config == 1)
	{
		if (i > 2)
			close(fds[i - 2][0]);
		close(fds[i][1]);
	}
	else
	{
		close(fds[i][0]);
		if (i > 1)
			close(fds[i - 1][0]);
	}
}

void	ft_microshell(t_ast *ast, pid_t *child_pid, int **fds, char **envp)
{
	t_ast	*next;
	int		i;

	next = ast;
	i = -1;
	while (next)
	{
		i++;
		child_pid[i] = fork();
		if (child_pid[i] == -1)
		{
			free(child_pid);
			free(fds);
			ft_error("error: fatal\n", ast);
		}
		else if (child_pid[i] == 0)
			ft_child(next, fds, envp, i);
		else
			close_fds_in_parent(fds, i, 1);
		next = ast->next;
	}
	close_fds_in_parent(fds, i, 0);
	wait_pid(child_pid);
}

int	main(int ac, char **av, char **envp)
{
	t_ast	*ast;
	pid_t	*child_pid;
	int		**fds;

	ast = build_ast(ac, av);
	if (!n_pid)
		return (0);
//	print_ast(ast);
	child_pid = malloc(sizeof(pid_t) * n_pid);
	if (!child_pid)
			ft_error("error: fatal\n", ast);
	fds = init_fds(n_pid);
	if (!fds)
	{
			free(child_pid);
			ft_error("error: fatal\n", ast);
	}
	ft_microshell(ast, child_pid, fds, envp);
	free_ast(ast);
	free_fds(fds);
	free(child_pid);
	return (g_status);
}
