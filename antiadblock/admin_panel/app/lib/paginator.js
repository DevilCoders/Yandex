// https://github.com/deoxxa/paginator
export default function paginator(perPage, length, totalResults, currentPage) {
    let totalPages = Math.ceil(totalResults / perPage),
        firstPage,
        lastPage;

    currentPage = parseInt(currentPage, 10) || 1;

    // Проверки на выход за границу
    if (currentPage < 1) {
        currentPage = 1;
    }
    if (currentPage > totalPages) {
        currentPage = totalPages;
    }
    // Первая и последняя страницы, которые будут отображены.
    firstPage = Math.max(1, currentPage - Math.floor(length / 2));
    lastPage = Math.min(totalPages, currentPage + Math.floor(length / 2));

    // Это срабатывает, если мы находимся на одной из крайностей или рядом с ней;
    // у нас не будет достаточно ссылок на страницы. Нам нужно соответствующим образом скорректировать наши границы.
    if (lastPage - firstPage + 1 < length) {
        if (currentPage < (totalPages / 2)) {
            lastPage = Math.min(totalPages, lastPage + (length - (lastPage - firstPage)));
        } else {
            firstPage = Math.max(1, firstPage - (length - (lastPage - firstPage)));
        }
    }

    // Это срабатывает при нечетном количестве страниц.
    if (lastPage - firstPage + 1 > length) {
        if (currentPage > (totalPages / 2)) {
            firstPage++;
        } else {
            lastPage--;
        }
    }

    return {
        totalPages: totalPages,
        currentPage: currentPage,
        firstPage: firstPage,
        lastPage: lastPage,
        previousPage: currentPage - 1,
        nextPage: currentPage + 1,
        hasPreviousPage: currentPage > 1,
        hasNextPage: currentPage < totalPages
    };
}
